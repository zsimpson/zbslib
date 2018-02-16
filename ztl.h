#ifndef ZTL_H
#define ZTL_H

#include "assert.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
#ifdef WIN32
	#include "malloc.h"
#endif

// ZTLVec is a growable vector used for objects for non-dynmically allocated objects
//------------------------------------------------------------------------------------------------------------------

template <class T>
class ZTLVec {
  public:
	int count;
	int alloc;
	int grow;
	T *vec;

	ZTLVec( int _alloc=0, int _grow=100 ) {
		count = 0;
		alloc = _alloc;
		grow = _grow;
		vec = 0;
		if( _alloc ) {
			vec = (T *)malloc( sizeof(T) * _alloc );
			memset( vec, 0, sizeof(T) * _alloc );
		}
	}

	~ZTLVec() {
		if( vec ) free( vec );
		vec = 0;
	}

	void reset() {
		setCount( 0 );
	}

	void clear() {
		if( vec ) {
			free( vec );
		}
		vec = 0;
		alloc = 0;
		count = 0;
		grow = 100;
	}

	void setCountToAtLeast( int _count ) {
		if( count < _count ) {
			setCount( _count );
		}
	}

	void setCount( int _count ) {
		if( _count < count ) {
			// Getting smaller
			memset( &vec[_count], 0, sizeof(T) * (count-_count) );
		}
		else if( _count > count ) {
			// Getting bigger
			if( _count > alloc ) {
				vec = (T *)realloc( vec, sizeof(T) * _count );
				alloc = _count;
			}
			memset( &vec[count], 0, sizeof(T) * (_count-count) );
		}
		count = _count;
	}

	void expandBy( int _count ) {
		setCount( count + _count );
	}

	int add( T &t ) {
		if( count >= alloc ) {
			// ALLOCATE more space
			alloc += grow;
			vec = (T *)realloc( vec, sizeof(T) * alloc );
			assert( vec );
			memset( &vec[count], 0, sizeof(T) * grow );
		}
		memcpy( &vec[count], &t, sizeof(T) );
		count++;
		return count-1;
	}

	int add( const T &t ) {
		if( count >= alloc ) {
			alloc += grow;
			vec = (T *)realloc( vec, sizeof(T) * alloc );
			assert( vec );
			memset( &vec[count], 0, sizeof(T) * grow );
		}
		memcpy( &vec[count], &t, sizeof(T) );
		count++;
		return count-1;
	}

	int add( ZTLVec<T> &t ) {
		int startI = count;
		setCount( count + t.count );
		memcpy( &vec[startI], t.vec, sizeof(T)*t.count );
		return count;
	}

	int addUniq( T &t ) {
		// SCAN the existing list for a duplicate
		for( int i=0; i<count; i++ ) {
			if( memcmp( &vec[i], &t, sizeof(T) ) == 0 ) {
				// Duplicate, do not add			
				return count-1;
			}
		}
	
		if( count >= alloc ) {
			// ALLOCATE more space
			alloc += grow;
			vec = (T *)realloc( vec, sizeof(T) * alloc );
			assert( vec );
			memset( &vec[count], 0, sizeof(T) * grow );
		}
		memcpy( &vec[count], &t, sizeof(T) );
		count++;
		return count-1;
	}

	void push( T &t ) {
		add( t );
	}

	T &pop() {
		int last = count-1;
		count--;
		return vec[last];
	}

	void del( int i ) {
		memmove( &vec[i], &vec[i+1], sizeof(T) * (count - i - 1) );
		memset( &vec[count-1], 0, sizeof(T) );
		count--;
	}

	void shiftLeft( int offset, int skip ) {
		// Shift from offset+skip to offset
		assert( offset + skip <= count );
		memmove( &vec[offset], &vec[offset+skip], sizeof(T) * (count-offset-skip) );
		memset( &vec[count-skip], 0, sizeof(T) * skip );
		count -= skip;
	}

	void rotateLeft( int index ) {
		assert( index < count );
		T* scratch = (T*)alloca( index * sizeof(T) );
		memcpy( scratch, vec, index * sizeof(T) );
		memcpy( vec, vec+index, (count-index)*sizeof(T) );
		memcpy( vec+count-index, scratch, index * sizeof(T) );
	}

	void expandAllocBy( int howMuch ) {
		int origAlloc = alloc;
		alloc += howMuch;
		vec = (T *)realloc(vec, sizeof(T) * alloc);
		assert( vec );
		memset( &vec[count], 0, sizeof(T) * (alloc-origAlloc) );
	}

	void set( int i, T src ) {
		if( i >= count ) {
			setCount( i+1 );
		}
		memcpy( &vec[i], &src, sizeof(T) );
	}

	T &operator [](int i) { return vec[i]; }

	T &get( int i ) { return vec[i]; }

	operator T *() { return vec; }

	void copy( ZTLVec<T> &t ) {
		clear();
		count = t.count;
		alloc = t.alloc;
		grow = t.grow;
		vec = (T *)malloc( sizeof(T)*alloc );
		memcpy( vec, t.vec, sizeof(T)*alloc );
	}

	void copy( T *t, int _count ) {
		clear();
		count = _count;
		alloc = _count;
		grow = _count > 10 ? _count : 10;
		vec = (T *)malloc( sizeof(T)*alloc );
		memcpy( vec, t, sizeof(T)*alloc );
	}

	void stealOwnershipFrom( ZTLVec<T> &src ) {
		clear();
		count = src.count;
		alloc = src.alloc;
		grow = src.grow;
		vec = src.vec;
		src.count = 0;
		src.alloc = 0;
		src.grow = 0;
		src.vec = 0;
	}

	int getIndex( T el ) {
		// Return an index, or -1 if this pointer isn't in here
		for( int i=0; i<count; i++) {
			if( vec[i] == el ) {
				return i;
			}
		}
		return -1;
	}

	void reverse() {
		T* scratch = (T*)alloca(count * sizeof(T));
		memcpy( scratch, vec, count * sizeof(T) );
		for( int i=0; i<count; i++ ) {
			set( i, scratch[count-i-1] );
		}
	}
};

// ZTLPVec is a growable vector like ZTLVec but expects dynamically allocated objects
// so that when a removal from the vector occurs, the object has delete called on it
// Example: 
//   ZTLPVec<Foo> list;
//   Foo *f = new Foo;
//   list.add( f );
//   list.del( 0 );  // "delete" will be called on the zero element cleaning it up properly
//------------------------------------------------------------------------------------------------------------------

template <class T>
class ZTLPVec {
  public:
	int count;
	int alloc;
	int grow;
	T **vec;

	ZTLPVec( int _alloc=0, int _grow=100 ) {
		count = 0;
		alloc = _alloc;
		grow = _grow;
		vec = 0;
		if( _alloc ) {
			vec = (T **)malloc( sizeof(T*) * _alloc );
			memset( vec, 0, sizeof(T*) * _alloc );
		}
	}

	~ZTLPVec() {
		clear();
	}

	void reset() {
		setCount( 0 );
	}

	void clear() {
		setCount( 0 );
		if( vec ) {
			free( vec );
			vec = 0;
		}
		alloc = 0;
		count = 0;
		grow = 100;
	}

	void clearWithoutDelete() {
		// Avoids deleting the items in the list.
		// See also setWithoutDelete()
		count = 0;
		clear();
	}

	void setCount( int _count ) {
		if( _count < count ) {
			// Getting smaller
			if( vec ) {
				for( int i=_count; i<count; i++ ) {
					if( vec[i] ) {
						delete vec[i];
						vec[i] = 0;
					}
				}
			}
		}
		else if( _count > count ) {
			// Getting bigger
			if( _count > alloc ) {
				vec = (T **)realloc( vec, sizeof(T*) * _count );
				alloc = _count;
			}
			memset( &vec[count], 0, sizeof(T*) * (_count-count) );
		}
		count = _count;
	}

	void expandBy( int _count ) {
		expandTo( this->count + _count );
	}

	int add( T *t ) {
		if( count >= alloc ) {
			// ALLOCATE more space
			alloc += grow;
			vec = (T **)realloc( vec, sizeof(T*) * alloc );
			memset( &vec[count], 0, sizeof(T*) * grow );
		}
		vec[count] = t;
		count++;
		return count-1;
	}

	/* (tfb) I added these but then didn't need them.
	int addUniq( T *t ) {
		for( int i=0; i<count; i++ ) {
			if( vec[i] == t ) {
				return i;
			}
		}
		return add( t );		
	}

	int addUniqString( T *t ) {
		// A handy 'specialization' in the case T=char and vec holds 0-terminated strings. (tfb)
		for( int i=0; i<count; i++ ) {
			if( !strcmp( t, vec[i] ) ) {
				return i;
			}
		}
		return add( t );
	}
	*/

	void push( T *t ) {
		add( t );
	}

	T *pop() {
		// This makes the caller responsible for deleting this pointer
		int last = count-1;
		count--;
		T *ret = vec[last];
		vec[last] = 0;
		return ret;
	}

	void removeWithoutDelete( int i ) {
		memmove( &vec[i], &vec[i+1], sizeof(T*) * (count - i - 1) );
		memset( &vec[count-1], 0, sizeof(T*) );
		vec[count-1] = 0;
		count--;
	}

	void del( int i ) {
		if( vec[i] ) {
			delete vec[i];
		}
		memmove( &vec[i], &vec[i+1], sizeof(T*) * (count - i - 1) );
		memset( &vec[count-1], 0, sizeof(T*) );
		vec[count-1] = 0;
		count--;
	}

	void del( T *t ) {
		for( int i=0; i<count; i++ ) {
			if( vec[i] == t ) {
				del( i );
				break;
			}
		}
	}

	void shiftLeft( int offset, int skip ) {
		// Shift from offset+skip to offset
		assert( offset + skip <= count );
		for( int i=offset; i<offset+skip; i++ ) {
			if( vec[i] ) {
				delete vec[i];
				vec[i] = 0;
			}
		}
		memmove( &vec[offset], &vec[offset+skip], sizeof(T*) * (count-offset-skip) );
		memset( &vec[count-skip], 0, sizeof(T*) * skip );
		count -= skip;
	}

	void rotateLeft( int index ) {
		assert( index < count );
		T* scratch = (T*)alloca( index * sizeof(T) );
		memcpy( scratch, vec, index * sizeof(T) );
		memcpy( vec, vec+index, (count-index)*sizeof(T) );
		memcpy( vec+count-index, scratch, index * sizeof(T) );
	}

	void expandAllocBy( int howMuch ) {
		int origAlloc = alloc;
		alloc += howMuch;
		vec = (T **)realloc(vec, sizeof(T*) * alloc);
		memset( &vec[count], 0, sizeof(T*) * (alloc-origAlloc));
	}

	void set( int i, T *src ) {
		if( vec[i] ) {
			delete vec[i];
			vec[i] = 0;
		}
		if( i >= count ) {
			this->setCount( i+1 );
		}
		vec[i] = src;
	}

	void setWithoutDelete(int i, T *src) {
		if (i >= count)
			this->setCount(i+1);
		vec[i] = src;
	}

	T *operator [](int i) { return vec[i]; }

	T *get( int i ) { return vec[i]; }

	void copy( ZTLPVec<T> &t ) {
		// This is probably the copy you want, if you want new copies of non-string
		// objects that will automatically get deleted upon destruction or replacement.
		clear();
		count = t.count;
		alloc = t.alloc;
		grow = t.grow;
		vec = (T **)malloc( sizeof(T*)*alloc );
		for( int i=0; i<count; i++ ) {
			vec[i] = 0;
			if( t.vec[i] ) {
				vec[i] = new T;
				memcpy( vec[i], t.vec[i], sizeof(T) );
			}
		}
	}

	void copyStrings( ZTLPVec<T> &t ) {
		// This is a 'specialization' of copy to be used when ZTLPVec holds pointers
		// to 0-terminted strings.  Note to really do a specialization for type 'char'
		// requires us to redefine *all* member fns, and as well rules out the use of
		// an array of pointers to single characters (ZTLPVec default behavior for char).
		assert( sizeof(T)==1 );
			// not bullet-proof, but helpful.
		clear();
		count = t.count;
		alloc = t.alloc;
		grow = t.grow;
		vec = (char **)malloc( sizeof(char*)*alloc );
		for( int i=0; i<count; i++ ) {
			vec[i] = strdup( t.vec[i] );
		}
	}	

	void copyPointers( ZTLPVec<T> &t ) {
		// This object must now take care to clearWithoutDelete() before
		// destruction, and otherwise ensure the objects held are not deleted
		// multiple times due to use of set() etc.
		clear();
		count = t.count;
		alloc = t.alloc;
		grow = t.grow;
		vec = (T **)malloc( sizeof(T*)*alloc );
		for( int i=0; i<count; i++ ) {
			vec[i] = t.vec[i];
		}
	}	

	void stealOwnershipFrom( ZTLPVec<T> &src ) {
		clear();
		count = src.count;
		alloc = src.alloc;
		grow = src.grow;
		vec = src.vec;
		src.count = 0;
		src.alloc = 0;
		src.grow = 0;
		src.vec = 0;
	}

	void addStealOwnershipFrom( ZTLPVec<T> &src ) {
		expandAllocBy( src.count );
		for( int i=0; i < src.count; i++ ) {
			add( src[i] );
		}
		memset( src.vec, 0, sizeof(T *)*src.count );
		src.clear();
	}

	// Insert at the given index
	void insert(int idx,T *ptr)
	{
		int oldCount = count;
		add(NULL);
		memmove (&vec[idx+1],&vec[idx],sizeof(T*)*(oldCount - idx));
		vec[idx] = ptr;
	}

	// Find item; return -1 on fail
	int find( T *t ) {
		for( int i=0; i<count; i++ ) {
			if( vec[i] == t ) {
				return i;
			}
		}
		return -1;
	}

	// Find item; return -1 on fail
	int getIndex( T *el ) {
		for( int i=0; i<count; i++) {
			if( vec[i] == el ) {
				return i;
			}
		}
		return -1;
	}

	void reverse() {
		T** scratch = (T**)alloca(count * sizeof(T*));
		memcpy( scratch, vec, count * sizeof(T*) );
		for( int i=0; i<count; i++ ) {
			setWithoutDelete( i, scratch[count-i-1] );
		}
	}


};

// ZTLStack is just like ZTLVec except stack LIFO
//------------------------------------------------------------------------------------------------------------------

template <class T>
class ZTLStack {
  public:
	int alloc;
	int grow;
	T *base;
	int top;

	ZTLStack( int _alloc=0, int _grow=100 ) {
		alloc = _alloc;
		grow = 100;
		top = 0;
		base = 0;
		if( _alloc ) {
			base = (T *)malloc( sizeof(T) * _alloc );
			memset( base, 0, sizeof(T) * _alloc );
		}
	}

	~ZTLStack() {
		if( base ) free( base );
		base = 0;
	}

	void clear() {
		top = 0;
		memset( base, 0, sizeof(T)*alloc );
	}

	void push( T t ) {
		if( top >= alloc ) {
			// ALLOCATE more space
			alloc += grow;
			base = (T *)realloc( base, sizeof(T) * alloc );
			memset( &base[top], 0, sizeof(T) * grow );
		}
		memcpy( &base[top], &t, sizeof(T) );
		top++;
	}

	T &pop() {
		top--;
		assert( top >= 0 );
		return base[top];
	}

	T &get( int i ) {
		assert( i >= 0 && i < top );
		return base[i];
	}

	int getIndex( T el ) {
		// Return an index, or -1 if not found
		for( int i=0; i<top; i++) {
			if( base[i] == el ) {
				return i;
			}
		}
		return -1;
	}

	T &getTop( int i=0 ) {
		assert( i >= 0 && i < top );
		return base[top-i-1];
	}
};

// ZTLList is a generic linked list, needs to the enumerator below to traverse efficiently
// If the "owner" flag is set, all objects are assumed to be dynamically allocated and are deleted on removal
// Example usage:
//   ZTLList<Thing> list;
//   list.addToTail( new Thing );
//------------------------------------------------------------------------------------------------------------------

template <class T>
struct ZTLList {
	struct Node {
		Node *next;
		Node *prev;
		T *t;
		Node() {
			next = prev = 0;
		}
	};

	Node *head;
	Node *tail;
	int owner;
		// If this is set, the del() also cause the ptr to be deleted
		// Default is FALSE

	void addToHead( T *t ) {
		Node *n = new Node;
		n->next = head;
		n->prev = 0;
		if( head ) head->prev = n;
		head = n;
		if( !tail ) tail = n;
		n->t = t;
	}

	void addToTail( T *t ) {
		Node *n = new Node;
		n->next = 0;
		n->prev = tail;
		if( tail ) tail->next = n;
		tail = n;
		if( !head ) head = n;
		n->t = t;
	}

	void moveToTail( T *t ) {
		Node *n = 0;
		for( n = head; n; n=n->next ) {
			if( n->t == t ) {
				break;
			}
		}
		if( n ) {
			if( n->prev ) n->prev->next = n->next;
			if( n->next ) n->next->prev = n->prev;
			if( head == n ) head = n->next;
			if( tail == n ) tail = n->prev;
			n->next = 0;
			n->prev = tail;
			if( tail ) tail->next = n;
			tail = n;
			if( !head ) head = n;
			n->t = t;
		}
	}

	void moveToHead( T *t ) {
		Node *n = 0;
		for( n = head; n; n=n->next ) {
			if( n->t == t ) {
				break;
			}
		}
		if( n ) {
			if( n->prev ) n->prev->next = n->next;
			if( n->next ) n->next->prev = n->prev;
			if( head == n ) head = n->next;
			if( tail == n ) tail = n->prev;
			n->next = head;
			n->prev = 0;
			if( head ) head->prev = n;
			head = n;
			if( !tail ) tail = n;
			n->t = t;
		}
	}

	void del( T *t ) {
		for( Node *n = head; n; n=n->next ) {
			if( n->t == t ) {
				if( n->prev ) n->prev->next = n->next;
				if( n->next ) n->next->prev = n->prev;
				if( head == n ) head = n->next;
				if( tail == n ) tail = n->prev;
				if( owner && n->t ) {
					delete t;
				}
				delete n;
				break;
			}
		}
	}

	void clear() {
		Node *next = 0;
		for( Node *n = head; n; ) {
			next = n->next;
			if( n->prev ) n->prev->next = n->next;
			if( n->next ) n->next->prev = n->prev;
			if( head == n ) head = n->next;
			if( tail == n ) tail = n->prev;
			if( owner && n->t ) {
				delete n->t;
			}
			delete n;
			n = next;
		}
	}

	ZTLList( int _owner=0 ) {
		head = tail = 0;
		owner = _owner;
	}

	~ZTLList() {
		clear();
	}
};

// ZTLListEnum safely enumeratoes over a list.  You safe delete the object during traversal
// Example usage:
//   ZTLList<Thing> list;
//   list.addToTail( new Thing );
//   for( ZTLListEnum<Thing> l(list); l.next(); ) {
//       list.del( l.cur );
//   }
//------------------------------------------------------------------------------------------------------------------


template <class T>
struct ZTLListEnum {
	typename ZTLList<T>::Node *nextPtr;
	typename ZTLList<T>::Node *curPtr;
	ZTLList<T> *list;
	T *cur;

	ZTLListEnum( ZTLList<T> &_list ) {
		list = &_list;
		nextPtr = (typename ZTLList<T>::Node *)-1;
		curPtr = 0;
		cur = 0;
	}

	int next() {
		if( !nextPtr ) return 0;
		if( nextPtr==(typename ZTLList<T>::Node *)-1 ) nextPtr = list->head;
		if( !nextPtr ) return 0;
		curPtr = nextPtr;
		nextPtr = curPtr->next;
		cur = curPtr->t;
		return 1;
	}
};

#endif

