//===- EDAN75.cpp ---------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/EDAN75.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

PreservedAnalyses EDAN75Pass::run(Function &F, FunctionAnalysisManager &AM) {
  for (BasicBlock &BB : F) {
    llvm::DenseMap<Value *, Value *> storedValues; // address -> value
    llvm::DenseSet<Value *> allocaAddresses;

    auto iter = BB.begin();
    while (iter != BB.end()) {
      Instruction &I = *iter;

      switch (I.getOpcode()) {
      case Instruction::Alloca: {
        allocaAddresses.insert(&I);
        iter++;
        break;
      }
      case Instruction::Store: {
        StoreInst &store = llvm::cast<StoreInst, Instruction>(I);
        Value *ptr = store.getPointerOperand();
        Value *val = store.getValueOperand();

        if (allocaAddresses.contains(ptr)) {
          storedValues.insert_or_assign(ptr, val);
          iter = store.eraseFromParent();
        } else {
          iter++;
        }
        break;
      }
      case Instruction::Load: {
        LoadInst &load = llvm::cast<LoadInst, Instruction>(I);
        Value *ptr = load.getPointerOperand();

        auto entry = storedValues.find(ptr);
        if (entry != storedValues.end()) {
          Value *alias = entry->getSecond();
          load.replaceAllUsesWith(alias);
          iter = load.eraseFromParent();
        } else {
          iter++;
        }
        break;
      }
      default:
        // check whether I is identical to an earlier instruction
        bool isDuplicate = false;
        for (auto iter2 = BB.begin(); iter2 != iter; iter2++) {
          Instruction &I2 = *iter2;
          if (I.isIdenticalTo(&I2)) {
            isDuplicate = true;
            I.replaceAllUsesWith(&I2);
            break;
          }
        }

        if (isDuplicate) {
          iter = I.eraseFromParent();
        } else {
          iter++;
        }
        break;
      }
    }

    // remove all instructions that are no longer needed
    iter = BB.begin();
    while (iter != BB.end()) {
      Instruction &I = *iter;
      if (I.isSafeToRemove() && !I.hasNUsesOrMore(1)) {
        iter = I.eraseFromParent();
      } else {
        iter++;
      }
    }
  }

  return PreservedAnalyses::none();
}
