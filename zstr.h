// @ZBS {
//		*MODULE_OWNER_NAME zstr
// }

#ifndef ZSTR_H
#define ZSTR_H

class ZHashTable;

// This is a simplistic implementation of Perl-like variables
// highly useful for quick string processing of files or other text.

struct ZStr {
	ZStr *next;
	char *s;
		// Always holds a private copy therefore must be removed on destruct

	ZStr( char *_s=(char*)0, int len=-1 );
	~ZStr();

	operator int();
	operator short();
	operator char();
	operator float();
	operator double();
	operator char *();
	ZStr *get(int x=0);
	char *getS(int i=0);
		// Return the ith string or empty;
	int is(int i,char *compare);
		// Compare the ith string to compare, return 1 on same
	int isFirst(char *compare) {
		return is( 0, compare );
	}
	int contains(int i,char *compare);
		// Compare the ith string to compare, return 1 if it contains the substring

	int getAsInt(int x=0);
		// Dpreceated, does the same thing as getI
	int getI(int x=0);
	float getF(int x=0);
	double getD(int x=0);

	int defined() { return s != (char*)0; }
	void set( char *_s, int len=-1 );

	int getLen();
	void appendS( char *str, int len=-1 );
	void prependS( char *str, int len=-1 );
};

ZStr *zStrSplitByRegExpPtr( void *_regExp, char *text );
	// Like Perl Split, takes an RegExp * as the first parameter
	// type cast here as a void * to avoid header dependency

ZStr *zStrSplit( char *regExp, char *text );
	// Like Perl split, takes the regular expression as a string

ZStr *zStrSplitByChar( char splitChar, char *text );
	// Split a line that is deliminated by a single char (like \t)

void zStrHashSplit( char *text, ZHashTable *hashTable );
	// splits the text into keyword value pairs and jams into the specified hashtable
	// eg: "name='Zack Booth' email='oink@oink.com'"
	// This is smart enough to deal with both single and double quotes as well as nested quotes
	// eg: "name='Zack\'s Room' email=\"\\\"Why Yes he said\\\"\"";

char *zStrJoin( char *seperator, ZStr *head );
	// Like perl join

int zStrCount( ZStr *head );
	// How many elements are in the linked list

void zStrDelete( ZStr *head );
	// delete an entire ZStr chain

char *zStrEscapeQuote( char *text );
	// escape any quotes with back-slashes.  Allocates a 2x buffer and returns it

void zStrChomp( char *str );
	// Like Perl chomp, removes trailing CRs and LFs

ZStr *zStrReplace( ZStr *lines, char *matchRegEx, char *replaceStr );
	// Like Perl chomp, removes trailing CRs and LFs

ZStr *zStrReadFile( char *file, char *eofTag=0 );
	// Read a whole file in translate mode creating a list of lines;
	// Optional eofTag will be treated as EOF marker.

ZStr *zStrAppendS( ZStr *&head, char *str, int len=-1 );
	// news a ZStr and adds it to the tail of the head list
	// if head is nul, it creates the head and sets head
	// This was called "AddToTail"

ZStr *zStrPrependS( ZStr *&head, char *str, int len=-1 );
	// news a ZStr and adds it to the head
	// if head is nul, it creates the head and sets head
	// This was called "AddToHead"

void zStrAppendList( ZStr *&head, ZStr *listToAppend );
	// Tack the "listToAppend" onto the tail of the "head" list.  This may cause head to change
	// This was called just "append"

void zStrPrependList( ZStr *&head, ZStr *listToAppend );
	// Tack the "listToAppend" onto the head of the "head" list.  This may cause head to change
	// This was called just "prepend"

ZStr *zStrCopy( ZStr *head );
	// Make a copy of the entire list starting at head

void zStrSortStrVectorIgnoreCase( char **vectorOfStrings, int count );
	// Given a vector of char * it will sort the list

void zStrToLower( char *str );
	// Given a string it converts to all lower case 

void zStrStripWhitespaceFrontAndBack( char *str );
	// Remove whitespace from front and back

void zStrPrint( ZStr *head, char *delim );
	// Print the strings separated by delim


#endif
