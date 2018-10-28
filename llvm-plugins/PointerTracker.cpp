/*
 * PointerTracker.cpp
 *
 * 	Created on: 
 * 	Author : Vinod
 */
#define DEBUG_TYPE "pointertracker"

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/ADT/Statistic.h"

#include <string>
#include <regex>
#include <metadata.h>

#ifdef DANG_DEBUG
#define DEBUG_MSG(err) 	err
#else
#define DEBUG_MSG(err)
#endif

using namespace llvm;

STATISTIC(DangRegisterPtrCall, "Number of store instructions instrumented");
STATISTIC(DangRegisterPtrCallDot, "Number of store instructions instrumented for dot function");

struct PointerTracker : public FunctionPass {
	static char 		ID;
	bool 			initialized;
	Module 			*M;
	const DataLayout 	*DL; 
	
	Type* 			VoidTy;
	IntegerType 		*IntPtrTy;
	
	MemoryDependenceAnalysis *MD;
        AliasAnalysis            *AA;

 	/* Function void @dang_registerptr(i64, i64) */
	Constant 		*RegisterPtrFunc;
        
        int print_debug = 0;
        bool dot_func = false;

	PointerTracker() : FunctionPass(ID) {}

	virtual bool runOnFunction(Function &F) {
		if (!initialized) {
			doInitialization(F.getParent());
		}

		if (ISMETADATAFUNC(F.getName().str().c_str())) {
			return false;
		}
		MD = &getAnalysis<MemoryDependenceAnalysis>();
                AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

		DEBUG_MSG(errs() << "Function : " << F.getName().str().c_str() << "\n");
                std::smatch func_match;
                std::regex func_regex("\\.[0-9]+");
                if (std::regex_search(F.getName().str(), func_match, func_regex)) {
                    dot_func = true;
                } else {
                    dot_func = false;
                }
                print_debug = 0;

                /* Following code is added to debug particular function.
                 *
                 if (strstr(F.getName().str().c_str(), "Compute_Cone_Data") != NULL) {
                    errs() << "Function : " << F.getName().str().c_str() << "\n";
                    print_debug = 1;
                }
                 */

                /* This is one way to skip stack objects. 
                 * Collect all store instructions having stack pointer operand
                 * std::set<const Instruction*> stackStores;
                for (auto &bb : F) {
                    for (auto &i : bb) {
                        AllocaInst *AI = dyn_cast<AllocaInst>(&i);
                        if (!AI)
                            continue;
                        AccumulateStackStores(AI, stackStores);
                    }
                }
                 */
	
		/* Iterate over function instructions */
		for (inst_iterator I = inst_begin(F),  E = inst_end(F); I != E; ++I) {
			Instruction *Inst = &(*I);
			DEBUG_MSG(Inst->dump());
                        if (print_debug == 1) {
                            Inst->dump();
                        }

			/*
 			 * Here, we need not to have to track allocation calls (e.g. malloc)
 			 * Even if return value is saved in local(stack) memory.
 			 * Because, store instruction will be used to store value to
 			 * Heap/Global location.
 			 */
			if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
				Value *ptr_addr = Store->getPointerOperand();
				Value *object_addr = Store->getValueOperand();
				Type *Ty = object_addr->getType();
			        
				/* Check whether rsh is pointer.
                                 * If stack pointers are to be skipped, comment out isStackPointer.
                                 */
				if (!isPointerOperand(object_addr)) { //|| isStackPointer(ptr_addr)) {
					DEBUG_MSG(errs() << "RHS Not a pointer type OR LHS Stack pointer \n");
					continue;
				}

                                /* Comment out this condition, if stack objects has to be tracked.
                                 */                               
                                if (isStackPointer(object_addr)) {
                                    DEBUG_MSG(errs() << "Stack value object \n");
                                    continue;
                                }

                                /*
                                if (stackStores.count(Store)) {
                                    DEBUG_MSG(errs() << "Stack store instruction \n");
                                    continue;
                                }
                                */
                             
 				/* Check whether pointer is a function pointer. */
				PointerType *ValueType = dyn_cast_or_null<PointerType>(Ty);
				if (ValueType && isa<FunctionType>(ValueType->getElementType())) {
					DEBUG_MSG(errs() << "Store : Function pointer type \n");
					continue;
				}
                                
                                /* Check whether value operand is a global variable */
                                if (isGlobalPointer(object_addr)) {
                                    DEBUG_MSG(errs() << "Global Variable : Value operand \n");
                                    continue;
                                }
				
				/* Check whether pointer is updated just with memory object arithmatic */
				if (isSameLoadStore(ptr_addr, object_addr)) {
					DEBUG_MSG(errs() << "Store : Pointer update with same object \n");
					continue;
				}
                                
                                /* Check for constant null pointer */
                                if (isa<ConstantPointerNull>(object_addr)) {
                                        DEBUG_MSG(errs() << "Store : Object pointer with constant null value \n");
                                        continue;
                                }
				
				/* Get next instruction after store */
				Instruction *insertBeforeInstruction = Inst;
				BasicBlock::iterator nextIt(Inst);
				//++nextIt;             /* This register a function before store instruction */
				insertBeforeInstruction = &*nextIt;
				IRBuilder<> B(insertBeforeInstruction);
				instrumentStore(ptr_addr, object_addr, B);
			}
		}
		return true; /* Transforms code */
	}
	
	bool doInitialization(Module *Mod) {
		M = Mod;
		DL = &(M->getDataLayout());
		if (!DL) {
			report_fatal_error("Data layout required");
		}
		
		VoidTy = Type::getVoidTy(M->getContext());
		IntPtrTy = DL->getIntPtrType(M->getContext(), 0);

		std::string functionName = "inlinedang_registerptr";
		RegisterPtrFunc = M->getOrInsertFunction(functionName, VoidTy, IntPtrTy,
									IntPtrTy, NULL); 
		initialized = true;
		return false;
	}
	
	void getAnalysisUsage(AnalysisUsage &AU) const override {
                AU.addRequired<AAResultsWrapperPass>();
                AU.addRequired<MemoryDependenceAnalysis>();
                AU.addPreserved<AAResultsWrapperPass>();
                //AU.addPreserved<BasicAAWrapperPass>();
                //AU.addPreserved<GlobalsAAWrapperPass>();
                //AU.addPreserved<MemoryDependenceAnalysis>();
	}

private:
        void printFunctionInst(Function &F) {
            errs() << "Function : " << F.getName().str().c_str() << "\n";
            for (inst_iterator I = inst_begin(F),  E = inst_end(F); I != E; ++I) {
                Instruction *Inst = &(*I);
                Inst->dump(); 
            }
        }

	bool isSameLoadStore(Value *ptr_addr, Value *obj_addr) {
		if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(obj_addr)) {
			if (LoadInst *LI = dyn_cast<LoadInst>(GEP->getPointerOperand())) {
				if (ptr_addr == LI->getPointerOperand()) {
					return true;
				}
			}
		}
		return false;
	}

	/* TODO: Handle this case */
	bool instrumentVectorStore(Value *ptr_addr, Value *obj_addr, IRBuilder<> &B) {
		DEBUG_MSG(errs() << "Vector Store instrumentation \n");
		assert(obj_addr->getType()->isVectorTy() && "Store has to be vector type ");
		return true;
	}

        /* This function retrieves root address if subtraction is used in GEP.
         * It handles specific GCC case. GCC has below code.
         * ptr = (malloc() - constant)
         */
	inline Value* getBoundedAddr(Value *obj_addr) {
		if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(obj_addr)) {
			if (1 == GEP->getNumIndices()) {
				if (ConstantInt *C = dyn_cast<ConstantInt>(GEP->getOperand(1)))
					if (C && C->getValue().isNegative())
						return GEP->getOperand(0);
				if (Instruction *SubI = dyn_cast<Instruction>(GEP->getOperand(1))) {
					if (SubI->getOpcode() == Instruction::Sub) { 
						ConstantInt *C = dyn_cast<ConstantInt>(SubI->getOperand(0));
						if (C && C->isZero())
							return GEP->getOperand(0);
					}
				}
			} 
		} /*else if (PtrToIntInst *PII = dyn_cast<PtrToIntInst>(obj_addr)) {
                    //errs() << "PointerToInt instruction \n";
                    if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(PII->getOperand(0))) {
                        //errs() << "Get elements pointer instruction \n";
                        return getBoundedAddr(GEP->getOperand(0));
                    }
                } else if (BitCastInst *BC = dyn_cast<BitCastInst>(obj_addr)) {
                    if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(BC->getOperand(0))){ 
                        return getBoundedAddr(GEP->getOperand(0));
                    }
                }*/
                //return GetUnderlyingObject(obj_addr, *DL);
                return obj_addr;
	}

        /* Instrucment store instruction */
	bool instrumentStore(Value *ptr_addr, Value *obj_addr, IRBuilder<> &B) {
                /* TODO: Remove this */
                if (isa<ConstantPointerNull>(obj_addr)) {
                    DEBUG_MSG(errs() << "Value is constant expression \n");
                    obj_addr->dump();
                }

		Type *objTy = obj_addr->getType();
		/*	
		if (!objTy->isFirstClassType() || objTy->isAggregateType())
			return false;
		*/

		if (objTy->isVectorTy()) {
			return instrumentVectorStore(ptr_addr, obj_addr, B);
		}

		std::vector<Value *> callArgs;
		if (DL->getTypeStoreSizeInBits(objTy) == DL->getPointerSizeInBits()) {
			/* GCC store 32 bit number in the pointer address */
			/* GCC stores calloc pointer by subtracting it with fixed constant.
 			 * i.e. stores negative out-of-bound pointer.
 			 */
			/*
 			 * If Object type is 64 bits but not pointer or integer type. It is mostly
 			 * of double type. Thus, explicitely cast it to i64. 
 			 */
			Value *obj_bound_addr;
			if (!(objTy->getScalarType()->isPointerTy() || 
					objTy->getScalarType()->isIntegerTy())) {
				DEBUG_MSG(errs() << "Not a correct type \n");
				obj_bound_addr = B.CreateBitCast(obj_addr, B.getInt64Ty());
			} else {
				obj_bound_addr = getBoundedAddr(obj_addr);
			}
			callArgs.push_back(B.CreatePtrToInt(ptr_addr, IntPtrTy));
			callArgs.push_back(B.CreatePtrToInt(obj_bound_addr, IntPtrTy));
			B.CreateCall(RegisterPtrFunc, callArgs);
			++DangRegisterPtrCall;
                        if (dot_func) {
                            ++DangRegisterPtrCallDot;
                        }
			return true;
		}
		return false;
	}
	
        /*
         * Check whether value is a stack variable.
         */	
	bool isStackPointer(Value *V) {
		/* TODO: Accurate backtracking needed */
		if (isa<AllocaInst>(V)) {
			DEBUG_MSG(errs() << "Stack variable \n");
			return true;
		}
		
		if (BitCastInst *BC = dyn_cast<BitCastInst>(V)) {
			return isStackPointer(BC->getOperand(0));
		} else if (PtrToIntInst *PI = dyn_cast<PtrToIntInst>(V)) {
			return isStackPointer(PI->getOperand(0));
		} else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
			return isStackPointer(GEP->getPointerOperand());
		}

		DEBUG_MSG(errs() << "Not a Stack variable \n");
		return false;
	}

        void AccumulateStackStores(const Instruction *AI, std::set<const Instruction*> &stackStores) {
            errs() << "AccumulateStackStores : " << *AI << "\n";

            SmallPtrSet<const Value *, 16> Visited;
            SmallVector<const Instruction *, 16> WorkList;
            WorkList.push_back(AI);
            while (!WorkList.empty()) {
                const Instruction *V = WorkList.pop_back_val();
                for (const Use &UI : V->uses()) {
                    auto I = cast<const Instruction>(UI.getUser());
                    assert(V == UI.get());
            
                    switch (I->getOpcode()) {
                        case Instruction::Store:
                            {
                                const StoreInst *SI = dyn_cast<StoreInst>(I);
                                if (AI == SI->getPointerOperand()) {
                                    stackStores.insert(I);
                                    DEBUG_MSG(errs() << "Ignoring this store : " << *I << "\n"); 
                                }
                            }
                                break;
                        case Instruction::PtrToInt:
                        case Instruction::IntToPtr:
                        case Instruction::BitCast:
                        case Instruction::Select: 
                        case Instruction::GetElementPtr:
                                if (Visited.insert(I).second)
                                    WorkList.push_back(cast<const Instruction>(I));
                                break;
                    }
                }
            }
        }
   
        /*
         * Check whether value is a global variable.
         */ 
        bool isGlobalPointer(Value *V) {
            V = V->stripPointerCasts();
            if (isa<GlobalValue>(V) || AA->pointsToConstantMemory(V)) {
                return true;
            }

            if (BitCastInst *BC = dyn_cast<BitCastInst>(V)) {
                return isGlobalPointer(BC->getOperand(0));
            } else if (PtrToIntInst *PI = dyn_cast<PtrToIntInst>(V)) {
                return isGlobalPointer(PI->getOperand(0));
            } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
                return isGlobalPointer(GEP->getPointerOperand());
            } else if (LoadInst *LI = dyn_cast<LoadInst>(V)) {          /* TODO: Remove load instruction */
                if (AA->pointsToConstantMemory(LI->getOperand(0))) {
                    DEBUG_MSG(errs() << "Global constant load \n");
                    return true;
                }
            } else if (SelectInst *SI = dyn_cast<SelectInst>(V)) {
                return (isGlobalPointer(SI->getTrueValue()) && isGlobalPointer(SI->getFalseValue()));
            }
            return false;
        }

	/*
 	 * This function is mostly called on value operand of store instruction.
 	 */
	bool isPointerOperand(Value *V) {
		if (V->getType()->isPointerTy()) {
			DEBUG_MSG(errs() << "Direct Pointer Type \n");
			return true;
		}
		if (isa<PtrToIntInst>(V)) {
			DEBUG_MSG(errs() << "Pointer to int type cast \n");
			return true;
		}
		
		/* 
 		 * Load instruction with Pointer operand beign 
 		 * Bitcast or GEP.
 		 */	
		if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
			DEBUG_MSG(errs () << "Load Instruction \n");
			Value *LoadPtr = LI->getPointerOperand();
			if (isDoublePointer(LoadPtr)) {
				DEBUG_MSG(errs() << "Load double pointer type \n");
				return true;
			}
			Value *DeepLoadPtr = nullptr;
			Instruction *Inst = nullptr;
			/* Handle Constant Expression */
			if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LoadPtr)) {
				/* 
  				 * Note: Be careful with constant expression. getAsInstruction() creates
 				 * Dangling reference. TODO: remove this and operate only on CE
 				 */
				Inst = CE->getAsInstruction();
			} else {
				Inst = dyn_cast<Instruction>(LoadPtr);
			}
	
			if (const BitCastInst *BC = dyn_cast_or_null<BitCastInst>(Inst)) {
				DEBUG_MSG(errs() << "Bit cast deep instruction \n");
				DeepLoadPtr = BC->getOperand(0);
			} else if (GetElementPtrInst *GEPI = dyn_cast_or_null<GetElementPtrInst>(Inst)) {
				DEBUG_MSG(errs() << "Get Element constant expression \n");
				DeepLoadPtr = GEPI->getPointerOperand();
			}
			
			if (isa<ConstantExpr>(LoadPtr)) {
				Inst->dropAllReferences();
			}

			if (DeepLoadPtr && isDoublePointer(DeepLoadPtr)) {
				DEBUG_MSG(errs() << "Double pointer type \n");
				return true;
			}
		}
		return false;
	}
	
	bool isDoublePointer(Value *V) {
		const Type* T = V->getType();
		return T->isPointerTy() && T->getContainedType(0)->isPointerTy(); 
	}
	
	/* TODO : Following Functions to be removed 	
	Value* createGEPInstFromConst(IRBuilder<> &B, ConstantExpr *ce) {
		SmallVector<Value*,4> ValueOperands;
		unsigned NumOperands = ce->getNumOperands();
		
		for (unsigned i = 0; i < NumOperands; ++i) {
			Value *Operand = ce->getOperand(i);
			Operand->dump();
			ValueOperands.push_back(Operand);
		}
		Value* gepi = 0;
		if (cast<GEPOperator>(ce)->isInBounds()) {
			gepi = B.CreateInBoundsGEP(cast<GEPOperator>(ce)->getSourceElementType(),
					ValueOperands[0],  makeArrayRef(&ValueOperands[1], NumOperands - 1), "dang.GEP");
		} else {
			gepi= B.CreateGEP(cast<GEPOperator>(ce)->getSourceElementType(),
					ValueOperands[0],  makeArrayRef(&ValueOperands[1], NumOperands - 1), "dang.GEP");
		}	
		gepi->dump();
		return gepi;
	}
	Value *elimConstExpr(Value *V) {
		if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
			if (Instruction::isBinaryOp(CE->getOpcode()))
				return V;
			assert((CE->getOpcode() == llvm::Instruction::GetElementPtr ||
				CE->isCast()) && "Only GEP or CAST const expressions are supported for now.");
			return elimConstExpr(CE->getOperand(0));
		}
		return V;
	}	
	*/
};

char PointerTracker::ID = 0;
static RegisterPass<PointerTracker> X("pointertracker", "Pointer Tracker Pass",
					true,
					false);
