#include <string>
#include <sstream>

extern "C" {
#include <clang-c/Index.h>
}

#include <gzstream.h>

#include "ClicDb.h"
#include "types.h"
#include "clic_printer.h"
#include "clic_parser.h"

// This code intentionally leaks memory like a sieve because the program is shortlived.

class IVisitor {
public:
    virtual enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent) = 0;
};

class EverythingIndexer : public IVisitor {
public:
    EverythingIndexer(const char* translationUnitFilename)
        : translationUnitFilename(translationUnitFilename) {}

    virtual enum CXChildVisitResult visit(CXCursor cursor, CXCursor parent) {
        CXFile file;
        unsigned int line, column, offset;
        clang_getInstantiationLocation(
                clang_getCursorLocation(cursor),
                &file, &line, &column, &offset);
        CXCursorKind kind = clang_getCursorKind(cursor);
        const char* cursorFilename = clang_getCString(clang_getFileName(file));

        if (!clang_getFileName(file).data || translationUnitFilename == cursorFilename) {
            return CXChildVisit_Continue;
        }

        CXCursor refCursor = clang_getCursorReferenced(cursor);
        if (!clang_equalCursors(refCursor, clang_getNullCursor())) {
            CXFile refFile;
            unsigned int refLine, refColumn, refOffset;
            clang_getInstantiationLocation(
                    clang_getCursorLocation(refCursor),
                    &refFile, &refLine, &refColumn, &refOffset);

            if (clang_getFileName(refFile).data) {
                std::string referencedUsr(clang_getCString(clang_getCursorUSR(refCursor)));
                if (!referencedUsr.empty()) {
                    std::stringstream ss;
                    ss << cursorFilename
                       << ":" << line << ":" << column << ":" << kind;
                    std::string location(ss.str());
                    usrToReferences[referencedUsr].insert(location);
                }
            }
        }
        return CXChildVisit_Recurse;
    }

    std::string translationUnitFilename;
    ClicIndex usrToReferences;
};

enum CXChildVisitResult visitorFunction(
        CXCursor cursor,
        CXCursor parent,
        CXClientData clientData)
{
    IVisitor* visitor = (IVisitor*)clientData;
    return visitor->visit(cursor, parent);
}

const char *prg = "";

void usage() {
    std::cerr << "Usage:\n"
        << "\t" << prg << " add   <dbFilename> <indexFilename> [<options>] <sourceFilename>\n"
        << "\t" << prg << " rm    <dbFilename> <indexFilename>\n"
        << "\t" << prg << " clear <dbFilename>\n";
}

int main_rm(int argc, const char* argv[]) {
    if (argc != 3) {
        usage();
        return 1;
    }

    ClicDb db(argv[2]);

    igzstream in(argv[3]);
    if (!in.good()) {
        std::cerr << "ERROR: Opening file `" << argv[2] << "'.\n";
        return 1;
    }

    for (const auto &it : istream_range(in)) {
        db.rmMultiple(it.first, it.second);
    }
    return 0;
}

int main_clear(int argc, const char* argv[]) {
    if (argc != 3) {
        usage();
        return 1;
    }

    ClicDb db(argv[2]);
    db.clear();
    return 0;
}

int main_add(int argc, const char* argv[]) {
    prg = argv[0];
    if (argc < 5) {
        usage();
        return 1;
    }

    const char* dbFilename = argv[2];
    const char* indexFilename = argv[3];
    const char* sourceFilename = argv[argc-1];

    // Set up the clang translation unit
    CXIndex cxindex = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        cxindex, 0,
        argv + 2, argc - 2, // Skip over dbFilename and indexFilename
        0, 0,
        CXTranslationUnit_None);

    // Print any errors or warnings
    int n = clang_getNumDiagnostics(tu);
    if (n > 0) {
        int nErrors = 0;
        for (int i = 0; i != n; ++i) {
            CXDiagnostic diag = clang_getDiagnostic(tu, i);
            CXString string = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
            fprintf(stderr, "%s\n", clang_getCString(string));
            if (clang_getDiagnosticSeverity(diag) == CXDiagnostic_Error
                    || clang_getDiagnosticSeverity(diag) == CXDiagnostic_Fatal)
                nErrors++;
        }
    }

    // Create the index
    EverythingIndexer visitor(sourceFilename);
    clang_visitChildren(
            clang_getTranslationUnitCursor(tu),
            &visitorFunction,
            &visitor);
    ClicIndex& index = visitor.usrToReferences;

    // OK, now write the index to a compressed file
    ogzstream out(indexFilename);
    if (!out.good()) {
        std::cerr << "ERROR: Opening file `" << argv[2] << "'.\n";
        return 1;
    }
    printIndex(out, index);

    // Now open the database and add the index to it
    ClicDb db(dbFilename);

    for (const auto &it : index) {
        const std::string& usr = it.first;
        db.addMultiple(usr, it.second);
    }

    return 0;
}

int main(int argc, const char* argv[]) {
    prg = argv[0];

    if (argc < 2) {
        usage();
        return 0;
    }

    const char *cmd = argv[1];

    if (std::string("add") == cmd)
        return main_add(argc, argv);

    if (std::string("rm") == cmd)
        return main_rm(argc, argv);

    if (std::string("clear") == cmd)
        return main_clear(argc, argv);

    usage();
    return 1;
}
