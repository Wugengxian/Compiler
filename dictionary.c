#include "dictionary.h" 
#include "string.h"
#include "stdlib.h" 
#include "symbols.h" 
unsigned hash(char *s)

{

    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)

      hashval = *s + 31 * hashval;

    return hashval % HASHSIZE;

}

char* dic_temp(){
  char* name = (char *)malloc(sizeof(char)*32);
	strcpy(name,"v");
	char temp[20];
	itoa(dic_count,temp,10);
	dic_count++;
	strcat(name,temp);
  return name;
}

/* lookup: look for s in hashtab */

struct nlist *lookup(char *s)

{

    struct nlist *np;

    for (np = hashtab[hash(s)]; np != NULL; np = np->next)

        if (strcmp(s, np->name) == 0)

          return np; /* found */

    return NULL; /* not found */

}

struct nlist *install(char *name, char *defn)

{

    struct nlist *np;

    unsigned hashval;

    if ((np = lookup(name)) == NULL) { /* not found */

        np = (struct nlist *) malloc(sizeof(*np));

        if (np == NULL || (np->name = strdup(name)) == NULL)

          return NULL;

        hashval = hash(name);

        np->next = hashtab[hashval];

        hashtab[hashval] = np;

    } else /* already there */

        free((void *) np->defn); /*free previous defn */

    if ((np->defn = strdup(defn)) == NULL)

       return NULL;

    return np;

}



char *strdup1(char *s) /* make a duplicate of s */

{

    char *p;

    p = (char *) malloc(strlen(s)+1); /* +1 for ¡¯\0¡¯ */

    if (p != NULL)

       strcpy(p, s);

    return p;

}