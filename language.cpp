#include "language.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexermakefile.h>

const QStringList cSuf = { "c" };
const QStringList cxxSuf = { "cpp", "cxx", "cc", "C" };
const QStringList hSuf = { "h", "hpp" };

Language* Language::query(QString path) {
	if (path[0] == '#') {
		return new CxxLanguage;
	}
	QString base = QFileInfo(path).baseName();
	QString suf = QFileInfo(path).suffix();
	if (QRegularExpression(R"(^[Mm]akefile$)").match(base).hasMatch()) {
		return new MakefileLanguage;
	} else if (cSuf.contains(suf)) {
		return new CLanguage;
	} else if (cxxSuf.contains(suf) || hSuf.contains(suf)) {
		return new CxxLanguage;
	} else {
		return new PlainLanguage;
	}
}

QsciLexer* PlainLanguage::lexer() const {
	return nullptr;
}

QsciLexer* MakefileLanguage::lexer() const {
	return new QsciLexerMakefile;
}

QsciLexer* CLanguage::lexer() const {
	return new QsciLexerCPP;
}

QsciLexer* CxxLanguage::lexer() const {
	return new QsciLexerCPP;
}
