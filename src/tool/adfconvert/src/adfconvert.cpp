/**
* Copyright (C) 2024-2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendAction.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "clang/AST/Decl.h"
#include "clang/Lex/PreprocessingRecord.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>

#include <unordered_map>
#include <unordered_set>

llvm::cl::opt<bool> rpuFlag("rpu", llvm::cl::desc("RPU flag"), llvm::cl::init(false));
llvm::cl::opt<std::string> outputFile("out", llvm::cl::desc("Specify the output file"), llvm::cl::value_desc("file"));

struct SourceLocationHash {  
    std::size_t operator()(const clang::SourceLocation& loc) const {  
        return std::hash<unsigned>()(loc.getRawEncoding());  
    }  
};  
     
struct SourceRangeHash {  
    std::size_t operator()(const clang::SourceRange& range) const {  
        std::size_t startHash = SourceLocationHash()(range.getBegin());  
        std::size_t endHash = SourceLocationHash()(range.getEnd());  
        return startHash ^ (endHash << 1);  
    }  
};  
  
struct SourceRangeEqual {  
    bool operator()(const clang::SourceRange& lhs, const clang::SourceRange& rhs) const {  
        return lhs.getBegin() == rhs.getBegin() && lhs.getEnd() == rhs.getEnd();  
    }  
}; 

class HeaderGraph {
public:
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacencyList;

    void createNode(const std::string& nodeName) {
        if (adjacencyList.find(nodeName) == adjacencyList.end()) {
            adjacencyList[nodeName] = std::unordered_set<std::string>();
        } 
    }

    void addEdge(const std::string& from, const std::string& to) {
        createNode(from); 
        createNode(to);  
        adjacencyList[from].insert(to);
    }
    
	bool nodeExists(const std::string& node) const {
        return adjacencyList.find(node) != adjacencyList.end();
    }

    bool existsPath(const std::string& start, const std::string& end) {
        if (adjacencyList.find(start) == adjacencyList.end() || adjacencyList.find(end) == adjacencyList.end()) {
            return false; 
        }
        
        std::unordered_set<std::string> visited; 
        return dfs(start, end, visited);
    }

    void printGraph() const {
        for (const auto& pair : adjacencyList) {
            std::cout << pair.first << ": ";
            for (const auto& neighbor : pair.second) {
                std::cout << neighbor << " ";
            }
            std::cout << std::endl;
        }
    }

private:
	    bool dfs(const std::string& current, const std::string& end, std::unordered_set<std::string>& visited) {
        if (current == end) {
            return true; 
        }
        
        visited.insert(current);
        
        for (const auto& neighbor : adjacencyList[current]) {
            if (visited.find(neighbor) == visited.end()) {
                if (dfs(neighbor, end, visited)) {
                    return true; 
                }
            }
        }
        return false; 
    }
};

using namespace clang;
using namespace clang::tooling;
class GlobalFunctionVisitor : public RecursiveASTVisitor<GlobalFunctionVisitor>
{
private:
	std::set<llvm::StringRef> headerNames;
	std::unordered_map<std::string, std::string> MemVars;
	std::unordered_map<std::string, int> varFlags;

	bool IsDerivedFromBaseClass(QualType Type, const std::string &BaseClassName)
	{
		// Retrieve the underlying type (usually a RecordType)
		if (const RecordType *RT = Type->getAs<RecordType>()){
			if (const CXXRecordDecl *RecordDecl = dyn_cast<CXXRecordDecl>(RT->getDecl())){
				// Check if the class has a base class named BaseClassName
				for (const CXXBaseSpecifier &BaseSpecifier : RecordDecl->bases()){
					QualType BaseType = BaseSpecifier.getType();
					if (const RecordType *BaseRT = BaseType->getAs<RecordType>()){
						if (const CXXRecordDecl *BaseRecordDecl = dyn_cast<CXXRecordDecl>(BaseRT->getDecl())){
							if (BaseRecordDecl->getNameAsString() == BaseClassName){
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	}

public:
	GlobalFunctionVisitor() : Context(nullptr), Rewrite(nullptr) {}
	explicit GlobalFunctionVisitor(Rewriter *Rewrite, ASTContext *Context)
		: Context(Context), Rewrite(Rewrite) {}

	std::unordered_map<std::string, std::string> graphNames;
	std::unordered_map<clang::SourceRange, std::pair<std::string, std::string>, SourceRangeHash, SourceRangeEqual> MemFuncs;
	// bool VisitCXXRecordDecl(CXXRecordDecl *Decl){
	// 	//forward and non-forward definition
	// 	auto Type = Context->getRecordType(Decl);
	// 	if(const RecordType *recordType = Type->getAs<RecordType>()){
	// 		if (const auto *recordDecl = dyn_cast<CXXRecordDecl>(recordType->getDecl())){
	// 			if ((recordDecl->getName() == "output_gmio") || ((recordDecl->getName() == "input_gmio"))){
	// 				SourceLocation loc = Decl->getLocation();
	// 				headerNames.insert(Context->getSourceManager().getFilename(loc));
	// 			}
	// 		}
	// 	}
	// 	return true;
	// }

	std::set<llvm::StringRef> getHeaderNames() const {
        return headerNames;
    }

	std::unordered_map<clang::SourceRange, std::pair<std::string, std::string>, SourceRangeHash, SourceRangeEqual> getMemFuncs() const {
        return MemFuncs;
    }

	std::unordered_map<std::string, std::string> getMemVars() const {
        return MemVars;
    }

	std::string GetArgument(const Expr *ArgExpr){
		std::string Result = "";
		if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(ArgExpr)){
			Result = DRE->getDecl()->getNameAsString();
		}
		else if (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(ArgExpr)){
			llvm::raw_string_ostream OS(Result);
			ASE->getBase()->printPretty(OS, nullptr, PrintingPolicy(Context->getLangOpts()));
		}
		else if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(ArgExpr)){
			if (UO->getOpcode() == UO_AddrOf){
				llvm::raw_string_ostream OS(Result);
				UO->getSubExpr()->printPretty(OS, nullptr, PrintingPolicy(Context->getLangOpts()));
			}
		}
		else if (const ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(ArgExpr)){
			Result = GetArgument(ICE->getSubExpr());
		}
		else{
			llvm::raw_string_ostream OS(Result);
			ArgExpr->printPretty(OS, nullptr, PrintingPolicy(Context->getLangOpts()));
		}
		return Result;
	};

	void printMemberExprChain(const Expr *Expr, std::vector<std::string> &names){
		if (const MemberExpr *Member = dyn_cast<MemberExpr>(Expr->IgnoreImplicit())){
			printMemberExprChain(Member->getBase(), names);
			names.push_back(Member->getMemberDecl()->getNameAsString());
		}

		else if(const ArraySubscriptExpr *Member = dyn_cast<ArraySubscriptExpr>(Expr->IgnoreImplicit())){
			printMemberExprChain(Member->getBase(), names);
			auto *Index = Member->getIdx()->IgnoreImpCasts();  

			std::string IndexStr;  
			llvm::raw_string_ostream Stream(IndexStr);  
			Index->printPretty(Stream, nullptr, PrintingPolicy(Context->getLangOpts()));  
 
			auto array = names.back() + "[" + Stream.str() + "]"; 
			names.pop_back();
			names.push_back(array);
		}
		
		else if (const DeclRefExpr *DeclRef = dyn_cast<DeclRefExpr>(Expr->IgnoreImplicit())){
			names.push_back(DeclRef->getNameInfo().getName().getAsString());;
		}
	}

	bool VisitCallExpr(const CallExpr *CE){

		const FunctionDecl *Callee = CE->getDirectCallee();
		if (Callee && (Callee->getNameAsString() == "free")){
			
			std::ostringstream ostr;
			const Expr *firstArgExpr = CE->getArg(0)->IgnoreImpCasts();
			SourceLocation StartLoc = CE->getBeginLoc();
			SourceLocation EndLoc = CE->getEndLoc();
			ostr << ".free(" <<GetArgument(firstArgExpr) <<")";
			MemFuncs.insert({SourceRange(StartLoc, EndLoc), {GetArgument(firstArgExpr), ostr.str()}});
		}
		return true;
	}

	bool VisitVarDecl(VarDecl *VD){

		auto rpu_str = (rpuFlag)?(", true") : "";
		
		if (VD->isFileVarDecl()){
			auto clsname = VD->getNameAsString();

			QualType VarType = VD->getType();			
			if (IsDerivedFromBaseClass(VarType, "graph")){

				std::string graphName  = VD->getName().str();
				auto index = graphNames.size();
				const std::string newAdfrGraphName = "gr" + ((index==0)?(""):(std::to_string(index)));
				graphNames.insert({graphName, newAdfrGraphName});

				SourceLocation StartLoc = VD->getBeginLoc();
				SourceLocation EndLoc = VD->getEndLoc();

				std::string NewDeclaration =  
											"#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))\n" +
											 VarType.getAsString() + " " + VD->getNameAsString() + "; \n" +
											 "#endif\n" +
											 "AbrGraph " + newAdfrGraphName +"(" + "\"" + VD->getNameAsString() + "\""+
											rpu_str +
											")"; 

				Rewrite->ReplaceText(SourceRange(StartLoc, EndLoc), NewDeclaration);
			}
		}

		return true; 
	}

	bool VisitCXXMemberCallExpr(CXXMemberCallExpr *MemberCall){
		
		if (CXXMethodDecl *MethodDecl = MemberCall->getMethodDecl()){
			if (MethodDecl->getNameAsString() == "wait"){
				const ValueDecl* memberDecl;
				std::vector<std::string> names;
				if (const Expr *Callee = MemberCall->getCallee()){
					if (const MemberExpr *Member = dyn_cast<MemberExpr>(Callee->IgnoreImplicit())){
						if(const DeclRefExpr *DeclCallee = dyn_cast<DeclRefExpr>(Member->getBase()->IgnoreImplicit())){
							memberDecl = DeclCallee->getDecl();
							auto memberType = memberDecl->getType();
							if ((IsDerivedFromBaseClass(memberType, "graph"))){
								std::ostringstream ostr;
								auto graphName  = memberDecl->getName().str();
								const std::string newAdfrGraphName = graphNames[graphName];
								if (MemberCall->getNumArgs() > 0){
									ostr << newAdfrGraphName << ".wait(";
									const Expr *ArgExpr = MemberCall->getArg(0);
									ostr << GetArgument(ArgExpr)<< ")";
								}
								else{
									ostr << newAdfrGraphName << ".wait()";
								}
								SourceLocation StartLoc = MemberCall->getBeginLoc();
								SourceLocation EndLoc = MemberCall->getEndLoc();
								Rewrite->ReplaceText(SourceRange(StartLoc, EndLoc), ostr.str());
								return true;					
							}
							else{
								return true;
							}
						}
					
						printMemberExprChain(Callee, names);
						int vecSize = names.size();
						SourceLocation StartLoc = MemberCall->getBeginLoc();
						SourceLocation EndLoc = MemberCall->getEndLoc();

						std::ostringstream ostr;
						auto graphName  = names[0];
						const std::string newAdfrGraphName = graphNames[graphName];
						if (MemberCall->getNumArgs() > 0){
							ostr << newAdfrGraphName << ".wait(\"";
							for(int i = 0; i<vecSize - 2; i++){
								ostr<<names[i] << ".";
							}
							ostr<<names[vecSize - 2]<<"\",\"";
							const Expr *ArgExpr = MemberCall->getArg(0);
							ostr << GetArgument(ArgExpr)<< "\")";
						}
						else{
							ostr << newAdfrGraphName << ".gmio_wait(\"";
							for(int i = 0; i<vecSize - 2; i++){
								ostr<<names[i] << ".";
							}
							ostr<<names[vecSize - 2]<<"\")";
						}
						Rewrite->ReplaceText(SourceRange(StartLoc, EndLoc), ostr.str());
					}
				}
				
			}
			else if((MethodDecl->getNameAsString() == "resume") || (MethodDecl->getNameAsString() == "init") ||(MethodDecl->getNameAsString() == "end") || (MethodDecl->getNameAsString() == "run")){
				
				const ValueDecl* memberDecl;
				if (const Expr *Callee = MemberCall->getCallee()){
					if (const MemberExpr *Member = dyn_cast<MemberExpr>(Callee->IgnoreImplicit())){
						if(const DeclRefExpr *DeclCallee = dyn_cast<DeclRefExpr>(Member->getBase()->IgnoreImplicit())){
							memberDecl = DeclCallee->getDecl();
							auto memberType = memberDecl->getType();
							if (!(IsDerivedFromBaseClass(memberType, "graph"))){
								return true;
							}
						}
					}
				}

				SourceLocation StartLoc = MemberCall->getBeginLoc();
				SourceLocation EndLoc = MemberCall->getEndLoc();

				std::ostringstream ostr;
				auto graphName  = memberDecl->getName().str();
				const std::string newAdfrGraphName = graphNames[graphName];
				if (MethodDecl->getNameAsString() == "init"){
					ostr << newAdfrGraphName << ".init()";
				}
				else if (MethodDecl->getNameAsString() == "resume"){
					ostr << newAdfrGraphName << ".resume()";
				}
				else if(MethodDecl->getNameAsString() == "end"){
					
					if (MemberCall->getNumArgs()>0){
						ostr << newAdfrGraphName << ".end(";
						const Expr *ArgExpr = MemberCall->getArg(0);
						ostr << GetArgument(ArgExpr)<< ")";
						}
					else{
						ostr << newAdfrGraphName << ".end()";
					}
				}
				else{
					if (MemberCall->getNumArgs() > 0){
						ostr << newAdfrGraphName << ".run(";
						const Expr *ArgExpr = MemberCall->getArg(0);
						ostr << GetArgument(ArgExpr)<< ")";
					}
					else{
						ostr << newAdfrGraphName << ".run()";					
					}
				}
				

				Rewrite->ReplaceText(SourceRange(StartLoc, EndLoc), ostr.str());

			}
			else if ((MethodDecl->getNameAsString() == "gm2aie_nb") || (MethodDecl->getNameAsString() == "aie2gm_nb") ||
			(MethodDecl->getNameAsString() == "gm2aie") || (MethodDecl->getNameAsString() == "aie2gm") ||
			(MethodDecl->getNameAsString() == "read") || (MethodDecl->getNameAsString() == "update") || (MethodDecl->getNameAsString() == "setAddress")){
				std::vector<std::string> names;

				if (const Expr *Callee = MemberCall->getCallee()){
					printMemberExprChain(Callee, names);
					SourceLocation StartLoc = MemberCall->getBeginLoc();
					SourceLocation EndLoc = MemberCall->getEndLoc();

					std::ostringstream ostr;
					int vecSize = names.size();
					auto graphName  = names[0];
					const std::string newAdfrGraphName = graphNames[graphName];
					ostr << newAdfrGraphName << "." << names[vecSize-1] << "("<<"\"";
					for(int i = 0; i<vecSize - 2; i++){
						ostr<<names[i] << ".";
					}
					if((MethodDecl->getNameAsString() == "setAddress")){
						ostr<<names[vecSize - 2]<<"\",";
						for (unsigned i = 0; i < MemberCall->getNumArgs(); ++i)
						{
							const Expr *ArgExpr = MemberCall->getArg(i);
							ostr << GetArgument(ArgExpr);
							if (i == MemberCall->getNumArgs() - 1 ){
								ostr << ")";
							}
							else{
								ostr << ",";
							}
						}
					}
					else if ((MemberCall->getNumArgs() == 1) && ((MethodDecl->getNameAsString() == "gm2aie_nb") || (MethodDecl->getNameAsString() == "aie2gm_nb") || 
					(MethodDecl->getNameAsString() == "gm2aie") || (MethodDecl->getNameAsString() == "aie2gm"))){
						ostr<<names[vecSize - 2]<<"\",\"";
						const Expr *ArgExpr = MemberCall->getArg(0);
						ostr << GetArgument(ArgExpr)<< "\")";
					}
					else{
						ostr<<names[vecSize - 2]<<"\",reinterpret_cast<char*>(";

						for (unsigned i = 0; i < MemberCall->getNumArgs(); ++i)
						{
							const Expr *ArgExpr = MemberCall->getArg(i);
							ostr << GetArgument(ArgExpr);
							if (i == MemberCall->getNumArgs() - 1 && (i == 0)){
								ostr << "))";
							}
							else if(i == MemberCall->getNumArgs() - 1){
								ostr << ")";
							}
							else if(i==0){
								ostr << "),";
							}
							else{
								ostr << ",";
							}
						}
					}
					Rewrite->ReplaceText(SourceRange(StartLoc, EndLoc), ostr.str());

					if((MethodDecl->getNameAsString() == "gm2aie_nb") || (MethodDecl->getNameAsString() == "gm2aie") ||
					(MethodDecl->getNameAsString() == "aie2gm_nb") || (MethodDecl->getNameAsString() == "aie2gm") || (MethodDecl->getNameAsString() == "setAddress"))
					{
						auto name = GetArgument(MemberCall->getArg(0));
						std::string found;
						for(auto var : MemFuncs){
							if (varFlags[var.second.first] != 1){
								if (name.find(var.second.first) != std::string::npos){
									varFlags[var.second.first] == 1;
									found  = var.second.first;
								}
							}
						}
						MemVars.insert({found, names[0]});
					}
				}
			}
		}
		return true; 
	}

	bool VisitCXXMethodDecl(CXXMethodDecl *MethodDecl){
		//cannot remove input_gmio class declaration as discussed because we can have function def in another header file with no include
		//REMARK: we said before that we can assume that all funcs are coming from a single header file adf.h
		// This is the assumption here not to remove anything that has init, end, wait, run 
		if ((MethodDecl->getNameAsString() == "gm2aie_nb") || (MethodDecl->getNameAsString() == "aie2gm_nb") ||
		(MethodDecl->getNameAsString() == "gm2aie") || (MethodDecl->getNameAsString() == "aie2gm") ){
			auto memberType = MethodDecl->getThisType();
			if(memberType->isPointerType()){
				QualType underlyingType = memberType->getPointeeType();
				if(const RecordType *recordType = underlyingType->getAs<RecordType>()){
					if (const auto *recordDecl = dyn_cast<CXXRecordDecl>(recordType->getDecl())){
						if ((recordDecl->getName() == "GMIO")){
							SourceLocation loc = MethodDecl->getLocation();
							headerNames.insert(Context->getSourceManager().getFilename(loc));
						}
					}
				}
			}	
		}
		else if ((MethodDecl->getNameAsString()=="malloc") || ((MethodDecl->getNameAsString()=="free"))){
			if (MethodDecl->isStatic()) {
				auto parentClass = MethodDecl->getParent()->getTypeForDecl();
				if(const RecordType *recordType = parentClass->getAs<RecordType>()){
					if (const auto *recordDecl = dyn_cast<CXXRecordDecl>(recordType->getDecl())){
						if ((recordDecl->getName() == "GMIO")){
							SourceLocation loc = MethodDecl->getLocation();
							headerNames.insert(Context->getSourceManager().getFilename(loc));
						}
					}
				}				
			}
		}
		else if((MethodDecl->getNameAsString()=="init") || ((MethodDecl->getNameAsString()=="run"))){
			auto memberType = MethodDecl->getThisType();
			if(memberType->isPointerType()){
				QualType underlyingType = memberType->getPointeeType();
				if(const RecordType *recordType = underlyingType->getAs<RecordType>()){
					if (const auto *recordDecl = dyn_cast<CXXRecordDecl>(recordType->getDecl())){
						if ((recordDecl->getName() == "graph")){
							SourceLocation loc = MethodDecl->getLocation();
							headerNames.insert(Context->getSourceManager().getFilename(loc));
						}
					}
				}
			}			
		}
		return true;
	}


private:
	ASTContext *Context;
	Rewriter *Rewrite;
};
 

class MyMatchCallback : public ast_matchers::MatchFinder::MatchCallback {    
private:
	GlobalFunctionVisitor* Visitor;
public:  
	explicit MyMatchCallback (GlobalFunctionVisitor* Visitor) : Visitor(Visitor) {}
    std::string var;
	void run(const ast_matchers::MatchFinder::MatchResult &Result) override {    
        if (const DeclRefExpr *val = Result.Nodes.getNodeAs<DeclRefExpr>("eqVarName")) {  
			var = val->getDecl()->getNameAsString();
        }
        if (const VarDecl *val = Result.Nodes.getNodeAs<VarDecl>("directVarName")) {  
			var = val->getNameAsString();
        }   
		if (const CallExpr *CE = Result.Nodes.getNodeAs<CallExpr>("MallocCall")) {
            const FunctionDecl *Callee = CE->getDirectCallee();
			std::ostringstream ostr;
			const Expr *firstArgExpr = CE->getArg(0)->IgnoreImpCasts();
			SourceLocation StartLoc = CE->getBeginLoc();
			SourceLocation EndLoc = CE->getEndLoc();
			ostr << ".malloc(" <<Visitor->GetArgument(firstArgExpr) <<")";
			Visitor->MemFuncs.insert({SourceRange(StartLoc, EndLoc), {var, ostr.str()}}); 
        }  
    }    
};

class MyASTConsumer : public ASTConsumer
{
public:

	explicit MyASTConsumer(GlobalFunctionVisitor* Visitor)
		: Visitor(Visitor), MallocMatcher(createMatcher()) {}

	static ast_matchers::StatementMatcher createMatcher() {
		return ast_matchers::anyOf(
		ast_matchers::callExpr(ast_matchers::isExpansionInMainFile(), 
		       ast_matchers::hasAncestor(ast_matchers::binaryOperator(ast_matchers::hasOperatorName("="), 
			   ast_matchers::hasLHS(ast_matchers::ignoringParenCasts(ast_matchers::declRefExpr().bind("eqVarName"))))), 
			   ast_matchers::callee(ast_matchers::cxxMethodDecl(ast_matchers::hasName("malloc")))).bind("MallocCall"),
		ast_matchers::callExpr(ast_matchers::isExpansionInMainFile(), 
		       ast_matchers::hasAncestor(ast_matchers::varDecl().bind("directVarName")), 
			   ast_matchers::callee(ast_matchers::cxxMethodDecl(ast_matchers::hasName("malloc")))).bind("MallocCall")
		);		   
	}
	void HandleTranslationUnit(ASTContext &Context) override
	{
		runMatcher(Context);  
		SourceManager &SM = Context.getSourceManager();
		auto Decls = Context.getTranslationUnitDecl()->decls();
		for (auto &Decl : Decls)
		{
			FullSourceLoc FullLocation = Context.getFullLoc(Decl->getLocation());
			if (FullLocation.isValid() && (SM.getFileCharacteristic(FullLocation) == clang::SrcMgr::C_User)){
				Visitor->TraverseDecl(Decl);
			}
		}
	}

private:
    
	void runMatcher(ASTContext &Context) {  
        ast_matchers::MatchFinder Finder;  
        MyMatchCallback Callback(Visitor);  
        Finder.addMatcher(MallocMatcher, &Callback);  
        Finder.matchAST(Context);  
    } 
	
	ast_matchers::StatementMatcher MallocMatcher; 
	GlobalFunctionVisitor* Visitor;
};

class IncludeCallback : public clang::PPCallbacks
{
public:
	Rewriter &TheRewriter;
	clang::Preprocessor &Preprocessor;
    HeaderGraph &HG;
	std::unordered_map<std::string, clang::SourceRange> &mainHeaders;

    explicit  IncludeCallback(Rewriter &TheRewriter, clang::Preprocessor &PP, HeaderGraph &Graph, std::unordered_map<std::string, clang::SourceRange> &mainHeadersList) : TheRewriter(TheRewriter), Preprocessor(PP), HG(Graph), mainHeaders(mainHeadersList)  {}

	void InclusionDirective(clang::SourceLocation HashLoc,
							const clang::Token &IncludeTok,
							clang::StringRef FileName,
							bool IsAngled,
							clang::CharSourceRange FilenameRange,
							const clang::OptionalFileEntryRef File,
							clang::StringRef SearchPath,
							clang::StringRef RelativePath,
							const clang::Module *SuggestedModule,
							bool ModuleImported,
							SrcMgr::CharacteristicKind FileType) override
	{
		if(FileType == clang::SrcMgr::C_User){
			clang::SourceManager &sourceManager = Preprocessor.getSourceManager();
			std::string SourceFile = sourceManager.getFilename(HashLoc).str();  
			if (File){
				HG.addEdge(SourceFile, File->getName().str());
			}
			auto main = sourceManager.getFileEntryForID(sourceManager.getFileID(HashLoc));
			if (sourceManager.isMainFile(*main)){
				auto beginloc = FilenameRange.getBegin(); 
				auto endloc = FilenameRange.getEnd();
				beginloc = sourceManager.getSpellingLoc(beginloc).getLocWithOffset(-sourceManager.getSpellingColumnNumber(beginloc) + 1);  
				if(File){
					mainHeaders.insert({File->getName().str(), SourceRange(beginloc, endloc)});
				}
				
				if (FileName.starts_with("x")) {
				//REMARK: Add ifndef to each on its own because we cannot know for sure they are contiguous and 
				//there are no other headers in between. The check for this is complicated but doable if needed
					std::ostringstream ostr;
					ostr << "#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)\n" ;
					ostr << "#include \"" << FileName.str();
					ostr << "\"\n#endif";
					TheRewriter.ReplaceText(SourceRange(beginloc, endloc), ostr.str());
				}
			}
		}
	}

}; 

class MyFrontendAction : public ASTFrontendAction
{
public:
	HeaderGraph G;  
	std::unordered_map<std::string, clang::SourceRange> mainHeaders;
	std::unordered_map<std::string, clang::SourceRange> marked;

	MyFrontendAction(): FileMgr (std::make_unique<clang::FileManager>(clang::FileSystemOptions())){}

	bool BeginSourceFileAction(CompilerInstance &CI) override
	{
		TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		
        clang::Preprocessor &PP = CI.getPreprocessor();
		PP.addPPCallbacks(std::move(std::make_unique<IncludeCallback>(TheRewriter, PP, G, mainHeaders)));
		return true;
	}

	void EndSourceFileAction() override{
	
		for (const llvm::StringRef& headerName : Visitor.getHeaderNames()){
			for (const auto& node : mainHeaders) {
				if (!G.nodeExists(node.first)) {
					//throw std::runtime_error("Node \"" + node.first + "\" does not exist in the graph. CHECK YOUR CODE!");
					std::cout << "Node \"" + node.first + "\" does not exist in the graph. CHECK YOUR CODE!" << std::endl;
				}
				if (G.existsPath(node.first, headerName.str())) {
					marked.insert({node.first, node.second});
				}
			}
		}

		for (auto& node : marked){
			
			std::string path = node.first;
    		size_t pos = path.find_last_of('/');
    		std::string filename = path.substr(pos + 1);
			std::ostringstream ostr;
			ostr<< "#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))\n"
			<< "#include \"" << filename << "\"\n#endif";
			TheRewriter.ReplaceText(node.second, ostr.str());
		}

		std::unordered_map<std::string, std::string> memVars = Visitor.getMemVars();
		for(auto& func : Visitor.getMemFuncs()){
			auto it = memVars.find(func.second.first);
			std::string result;
			if ( it == memVars.end()){
				auto str = func.second.second;
				auto dotPos = str.find(".");
				auto parenPos = str.find("(");
				auto funcName = str.substr(dotPos + 1, parenPos - dotPos - 1);
				llvm::outs()<<"\033[31mCould not find corresponding graph for the " << funcName <<" call on the " << func.second.first <<" memory variable \033[0m\n"; 
				result = " " + func.second.second;
			}
			else{
				result = Visitor.graphNames[memVars[func.second.first]] + func.second.second;
			}
			TheRewriter.ReplaceText(func.first, result);			
		}

		std::cout<<"\n";
		TheRewriter.InsertText(TheRewriter.getSourceMgr().getLocForStartOfFile(TheRewriter.getSourceMgr().getMainFileID()), "#include \"aiebaremetal.h\"\n");
		const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(TheRewriter.getSourceMgr().getMainFileID());	

		auto &CI = getCompilerInstance(); 
        const auto &inputs = CI.getInvocation().getFrontendOpts().Inputs;  
		const auto &sourceFile = inputs.front(); 
		const auto &inputFilePath = sourceFile.getFile();  

		llvm::StringRef parentFilePath = llvm::sys::path::parent_path(inputFilePath);
		auto newName = parentFilePath.str();

		llvm::StringRef fileName = llvm::sys::path::filename(inputFilePath);
        const auto fileNameString = fileName.str();  
		char* fileNamePtr = (char*)fileNameString.c_str();

		std::vector<std::string> parts; 
		char* token = std::strtok(fileNamePtr, "."); 
		while (token != nullptr) {  
        	parts.push_back(token);   
        	token = std::strtok(nullptr, ".");

    	}   
		newName += ("/"+parts[0]+".new");  

		for (auto it = parts.begin() + 1; it!= parts.end(); it++) {  
			newName += ".";
			newName += *it;
    	}   
		
        std::error_code errorCode;
        if (!outputFile.empty()){
			newName = outputFile;
		}

		llvm::sys::fs::create_directories(llvm::sys::path::parent_path(newName));

        llvm::raw_fd_ostream outFile(newName, errorCode, llvm::sys::fs::OF_None);

        if (errorCode) {
            llvm::errs() << "Error opening file: " << errorCode.message() << "\n";
            return;
        }
		
		if (RewriteBuf){
			RewriteBuf->write(outFile);
		} 
        outFile.close();

	}
	
	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
												   StringRef file) override{
		Visitor = GlobalFunctionVisitor(&TheRewriter, &CI.getASTContext());
		return std::make_unique<MyASTConsumer>(&Visitor);
	}

private:
	std::string code;
	Rewriter TheRewriter;
	std::unique_ptr<clang::FileManager> FileMgr;
	GlobalFunctionVisitor Visitor;
};

int main(int argc, const char **argv)
{

	if (argc > 2){
		if (strcmp(argv[1], "pass") == 0){
			return 0;
		}
	}

	llvm::cl::OptionCategory MyToolCategory("acompiler options");
	auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, llvm::cl::OneOrMore);
	if (!ExpectedParser){
		llvm::errs() << "Error: " << llvm::toString(ExpectedParser.takeError()) << "\n";
		return 1;
	}
	CommonOptionsParser &OptionsParser = ExpectedParser.get();

	ClangTool Tool(OptionsParser.getCompilations(),
				   OptionsParser.getSourcePathList());

	return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
