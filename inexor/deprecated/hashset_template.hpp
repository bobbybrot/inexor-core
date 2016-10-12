#pragma once

#include "inexor/macros/memfree_macros.hpp"
#include "inexor/macros/loop_macros.hpp"
#include <algorithm>

namespace inexor {
namespace server {

    template<class H, class E, class K, class T> struct hashbase
    {
        typedef E elemtype;
        typedef K keytype;
        typedef T datatype;

        enum { CHUNKSIZE = 64 };

        struct chain 
        {
            E elem; 
            chain *next;
        };
        struct chainchunk 
        {
            chain chains[CHUNKSIZE];
            chainchunk *next;
        };

        int size;
        int numelems;
        chain **chains;

        chainchunk *chunks;
        chain *unused;

        enum { DEFAULTSIZE = 1<<10 };

        hashbase(int size = DEFAULTSIZE)
        : size(size)
        {
            numelems = 0;
            chunks = NULL;
            unused = NULL;
            chains = new chain *[size];
            memset(chains, 0, size*sizeof(chain *));
        }

        ~hashbase()
        {
            DELETEA(chains);
            deletechunks();
        }

        chain *insert(uint h)
        {
            if(!unused)
            {
                chainchunk *chunk = new chainchunk;
                chunk->next = chunks;
                chunks = chunk;
                loopi(CHUNKSIZE-1) chunk->chains[i].next = &chunk->chains[i+1];
                chunk->chains[CHUNKSIZE-1].next = unused;
                unused = chunk->chains;
            }
            chain *c = unused;
            unused = unused->next;
            c->next = chains[h];
            chains[h] = c;
            numelems++;
            return c;
        }

        template<class U>
        T &insert(uint h, const U &key)
        {
            chain *c = insert(h);
            H::setkey(c->elem, key);
            return H::getdata(c->elem);
        }

        #define HTFIND(success, fail) \
            uint h = hthash(key)&(this->size-1); \
            for(chain *c = this->chains[h]; c; c = c->next) \
            { \
                if(htcmp(key, H::getkey(c->elem))) return success H::getdata(c->elem); \
            } \
            return (fail);

        template<class U>
        T *access(const U &key)
        {
            HTFIND(&, NULL);
        }

        template<class U, class V>
        T &access(const U &key, const V &elem)
        {
            HTFIND( , insert(h, key) = elem);
        }

        template<class U>
        T &operator[](const U &key)
        {
            HTFIND( , insert(h, key));
        }

        template<class U>
        T &find(const U &key, T &notfound)
        {
            HTFIND( , notfound);
        }

        template<class U>
        const T &find(const U &key, const T &notfound)
        {
            HTFIND( , notfound);
        }

        template<class U>
        bool remove(const U &key)
        {
            uint h = hthash(key)&(size-1);
            for(chain **p = &chains[h], *c = chains[h]; c; p = &c->next, c = c->next)
            {
                if(htcmp(key, H::getkey(c->elem)))
                {
                    *p = c->next;
                    c->elem.~E();
                    new (&c->elem) E;
                    c->next = unused;
                    unused = c;
                    numelems--;
                    return true;
                }
            }
            return false;
        }

        void deletechunks()
        {
            for(chainchunk *nextchunk; chunks; chunks = nextchunk)
            {
                nextchunk = chunks->next;
                delete chunks;
            }
        }

        void clear()
        {
            if(!numelems) return;
            memset(chains, 0, size*sizeof(chain *));
            numelems = 0;
            unused = NULL;
            deletechunks();
        }

        static inline chain *enumnext(void *i) { return ((chain *)i)->next; }
        static inline K &enumkey(void *i) { return H::getkey(((chain *)i)->elem); }
        static inline T &enumdata(void *i) { return H::getdata(((chain *)i)->elem); }
    };

    template<class T> struct hashset : hashbase<hashset<T>, T, T, T>
    {
        typedef hashbase<hashset<T>, T, T, T> basetype;

        hashset(int size = basetype::DEFAULTSIZE) : basetype(size) {}

        static inline const T &getkey(const T &elem) { return elem; }
        static inline T &getdata(T &elem) { return elem; }
        template<class K> static inline void setkey(T &elem, const K &key) {}

        template<class V>
        T &add(const V &elem)
        {
            return basetype::access(elem, elem);
        }
    };

    template<class T> struct hashnameset : hashbase<hashnameset<T>, T, const char *, T>
    {
        typedef hashbase<hashnameset<T>, T, const char *, T> basetype;

        hashnameset(int size = basetype::DEFAULTSIZE) : basetype(size) {}

        template<class U> static inline const char *getkey(const U &elem) { return elem.name; }
        template<class U> static inline const char *getkey(U *elem) { return elem->name; }
        static inline T &getdata(T &elem) { return elem; }
        template<class K> static inline void setkey(T &elem, const K &key) {}

        template<class V>
        T &add(const V &elem)
        {
            return basetype::access(getkey(elem), elem);
        }
    };

    template<class K, class T> struct hashtableentry
    {
        K key;
        T data;
    };

    template<class K, class T> struct hashtable : hashbase<hashtable<K, T>, hashtableentry<K, T>, K, T>
    {
        typedef hashbase<hashtable<K, T>, hashtableentry<K, T>, K, T> basetype;
        typedef typename basetype::elemtype elemtype;

        hashtable(int size = basetype::DEFAULTSIZE) : basetype(size) {}

        static inline K &getkey(elemtype &elem) { return elem.key; }
        static inline T &getdata(elemtype &elem) { return elem.data; }
        template<class U> static inline void setkey(elemtype &elem, const U &key) { elem.key = key; }
    };

    #define enumeratekt(ht,k,e,t,f,b) loopi((ht).size) for(void *ec = (ht).chains[i]; ec;) { k &e = (ht).enumkey(ec); t &f = (ht).enumdata(ec); ec = (ht).enumnext(ec); b; }
    #define enumerate(ht,t,e,b)       loopi((ht).size) for(void *ec = (ht).chains[i]; ec;) { t &e = (ht).enumdata(ec); ec = (ht).enumnext(ec); b; }

    struct unionfind
    {
        struct ufval
        {
            int rank, next;

            ufval() : rank(0), next(-1) {}
        };

        vector<ufval> ufvals;

        int find(int k)
        {
            if(k>=ufvals.length()) return k;
            while(ufvals[k].next>=0) k = ufvals[k].next;
            return k;
        }

        int compressfind(int k)
        {
            if(ufvals[k].next<0) return k;
            return ufvals[k].next = compressfind(ufvals[k].next);
        }

        void unite (int x, int y)
        {
            while(ufvals.length() <= std::max(x, y)) ufvals.add();
            x = compressfind(x);
            y = compressfind(y);
            if(x==y) return;
            ufval &xval = ufvals[x], &yval = ufvals[y];
            if(xval.rank < yval.rank) xval.next = y;
            else
            {
                yval.next = x;
                if(xval.rank==yval.rank) yval.rank++;
            }
        }
    };

};
};