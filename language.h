#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <Qsci/qscilexer.h>

struct Language {
	virtual QsciLexer* lexer() const = 0;
	virtual ~Language() {}
	static Language* query(QString path);
	template <typename Type>
	bool test() const {
		return dynamic_cast<const Type*>(this);
	}
};

struct PlainLanguage : public Language {
	virtual QsciLexer* lexer() const;
};

struct MakefileLanguage : public Language {
	virtual QsciLexer* lexer() const;
};

struct CompilableLanguage : public Language {};

struct CLanguage : public CompilableLanguage {
	virtual QsciLexer* lexer() const;
};

struct CxxLanguage : public CompilableLanguage {
	virtual QsciLexer* lexer() const;
};


#endif // LANGUAGE_H
