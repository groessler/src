# $OpenBSD: Makefile,v 1.2 2017/01/14 20:12:41 patrick Exp $

LIB=	LLVMScalarOpts
NOPIC=
NOPROFILE=

CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Transforms
CPPFLAGS+=	-I${LLVM_SRCS}/include/llvm/Transforms/Scalar

.include <bsd.own.mk>
SRCS=	ADCE.cpp \
	AlignmentFromAssumptions.cpp \
	BDCE.cpp \
	ConstantHoisting.cpp \
	ConstantProp.cpp \
	CorrelatedValuePropagation.cpp \
	DCE.cpp \
	DeadStoreElimination.cpp \
	EarlyCSE.cpp \
	FlattenCFGPass.cpp \
	Float2Int.cpp \
	GVN.cpp \
	GVNHoist.cpp \
	GuardWidening.cpp \
	InductiveRangeCheckElimination.cpp \
	IndVarSimplify.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoadCombine.cpp \
	LoopDataPrefetch.cpp \
	LoopDeletion.cpp \
	LoopDistribute.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopInterchange.cpp \
	LoopLoadElimination.cpp \
	LoopRerollPass.cpp \
	LoopRotation.cpp \
	LoopSimplifyCFG.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LoopVersioningLICM.cpp \
	LowerAtomic.cpp \
	LowerExpectIntrinsic.cpp \
	MemCpyOptimizer.cpp \
	MergedLoadStoreMotion.cpp \
	NaryReassociate.cpp \
	PartiallyInlineLibCalls.cpp \
	PlaceSafepoints.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	RewriteStatepointsForGC.cpp \
	SCCP.cpp \
	SROA.cpp \
	Scalar.cpp \
	Scalarizer.cpp \
	SeparateConstOffsetFromGEP.cpp \
	SimplifyCFGPass.cpp \
	Sink.cpp \
	SpeculativeExecution.cpp \
	StraightLineStrengthReduce.cpp \
	StructurizeCFG.cpp \
	TailRecursionElimination.cpp

.PATH:	${.CURDIR}/../../../llvm/lib/Transforms/Scalar

depend:
	# Nothing here so far ...

install:
	# Nothing here so far ...

.include <bsd.lib.mk>
