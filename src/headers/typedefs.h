
#ifndef _H_DEFS
#define _H_DEFS

#define usize unsigned long

// Core
typedef struct Allocator Allocator;
typedef struct AllocatorBlock AllocatorBlock;
// Utils
typedef struct Array Array;
typedef struct Map Map;
typedef struct Str Str;
// Build
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;
typedef struct PkgConfig PkgConfig;
typedef struct Chunk Chunk;
typedef struct ChunkPos ChunkPos;
typedef struct Stage Stage;
typedef struct CompileData CompileData;
typedef struct Parser Parser;
typedef struct Unit Unit;
typedef struct Thread Thread;
typedef struct Link Link;
// IR
typedef struct IR IR;
typedef struct IRFunc IRFunc;
typedef struct IRBlock IRBlock;
typedef struct IRPhiValue IRPhiValue;
typedef struct IRFuncIR IRFuncIR;
// Parse
typedef struct Func Func;
typedef struct FuncArg FuncArg;
typedef struct FuncError FuncError;
typedef struct ErrorCollection ErrorCollection;
typedef struct Scope Scope;
typedef struct Idf Idf;
typedef struct Id Id;
typedef struct Type Type;
typedef struct Decl Decl;
typedef struct DeclOverwrite DeclOverwrite;
typedef struct Global Global;
typedef struct Class Class;
typedef struct ClassProp ClassProp;
typedef struct Trait Trait;
typedef struct ValueAlias ValueAlias;
typedef struct Alias Alias;
typedef struct Test Test;
// Tokens
typedef struct Token Token;
typedef struct TDeclare TDeclare;
typedef struct TIf TIf;
typedef struct TWhile TWhile;
typedef struct TEach TEach;
typedef struct TThrow TThrow;
typedef struct TSetRetv TSetRetv;
// Values
typedef struct Value Value;
typedef struct VPair VPair;
typedef struct VFuncPtr VFuncPtr;
typedef struct VFuncCall VFuncCall;
typedef struct VNumber VNumber;
typedef struct VGcBuffer VGcBuffer;
typedef struct VClassPA VClassPA;
typedef struct VPtrv VPtrv;
typedef struct VPtrOffset VPtrOffset;
typedef struct VOp VOp;
typedef struct VIncr VIncr;
typedef struct VIRCached VIRCached;
typedef struct VString VString;
typedef struct VPhiValue VPhiValue;
typedef struct VScope VScope;
typedef struct VVar VVar;
typedef struct VThisOrThat VThisOrThat;
// Snippet
typedef struct Snippet Snippet;
typedef struct SnipArg SnipArg;

#endif
