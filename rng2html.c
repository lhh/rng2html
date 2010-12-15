#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib.h>
#include "tree.h"


static xmlDocPtr 
open_relaxng(const char *filename)
{
	xmlDocPtr p;
	xmlNodePtr n;

	p = xmlParseFile(filename);
	if (!p) {
		printf("Failed to parse %s\n", filename);
	}

	n = xmlDocGetRootElement(p);
	if (xmlStrcmp(n->name, (xmlChar *)"grammar")) {
		printf("%s is not a relaxng grammar\n", filename);
		xmlFreeDoc(p);
		return NULL;
	}

	return p;
}


int
main(int argc, char **argv)
{
	GList *tree = NULL;
	GList *objs = NULL;
	xmlDocPtr doc;
	FILE *outfile;
	struct stat st_buf;

	if (argc < 2) {
		printf("Translate cluster RelaxNG -> HTML\n");
		printf("Usage: %s schema-file.rng [output-file.html]\n",
		       argv[0]);
		return 1;
	}

	doc = open_relaxng(argv[1]);
       	if (doc == NULL) {
		printf("Cannot continue\n");
		return 1;
	}

	find_objects(xmlDocGetRootElement(doc), NULL, &tree, &objs);

	if (argc == 2 || !strcmp(argv[2], "-")) {
		outfile = stdout;
	} else if (stat(argv[2], &st_buf) == 0) {
		printf("Please remove %s and rerun.\n", argv[2]);
		return 1;
	} else {
		outfile = fopen(argv[2], "w+");
	}

	fprintf(outfile, "<html>\n");
	print_tree(outfile, tree);

	print_tag_reference(outfile, objs);
	fprintf(outfile, "</html>\n");

	fclose(outfile);

	return 0;
}
