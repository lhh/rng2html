#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <assert.h>
#include <glib.h>

#include "zalloc.h"
#include "tree.h"
#include "debug.h"

char *normalize_space(char *);

struct print_info {
	FILE *fp;
	int tablevel;
};


#define RET(x) do { ret=x; goto out; } while (0)

int
compare_char(char l, char r)
{
	int ret;

	if (l == r)
		RET(0);
	if (isalpha(l) && isalpha(r)) {
		l|=32;
		r|=32;
		if (l == r)
			RET(0);
		if (l < r)
			RET(-1);
		RET(1);
	}
	if (isalnum(l) && !isalnum(r))
		RET(-1);
	if (!isalnum(l) && isalnum(r))
		RET(1);
	if (l < r)
		RET(-1);
	RET(1);
out:
	return ret;
}


int funky_strcmp(const char *left, const char *right)
{
	int ret;

	while (*left && *right) {
		ret = compare_char(*left, *right);
		if (ret)
			return ret;
		++left;
		++right;
	}

	if (*left)
		return 1;
	if (*right)
		return -1;
	return 0;
}




int
compare_attr(const void *left, const void *right)
{
	struct attr_node *l = (struct attr_node *)left;
	struct attr_node *r = (struct attr_node *)right;

	return funky_strcmp(l->name, r->name);
}


int
compare_element(const void *left, const void *right)
{
	struct element_node *l = (struct element_node *)left;
	struct element_node *r = (struct element_node *)right;

	return funky_strcmp(l->name, r->name);
}



static struct attr_node *
find_attr_byname(GList *attrs, char *name)
{
	struct attr_node foo;
	GList *gret;

	foo.name = name;

	dbg_printf("find attr byname name = %s\n", name);

	gret = g_list_find_custom(attrs, &foo, compare_attr);
	if (!gret)
		return NULL;
	return (struct attr_node *)(gret->data);
}


static struct attr_node *
get_attr(xmlNodePtr curr_node)
{
	struct attr_node *n;
	char *name, *desc;

	name = (char *)xmlGetProp(curr_node, (xmlChar *)"name");
	if (!name)
		return NULL;

	n = zalloc(sizeof(*n));
	n->name = name;
	n->this_node = curr_node;
	desc = xmlGetNsProp(curr_node, (xmlChar *)"description", RHA_URI);
	if (desc) {
		n->desc = strdup(desc);
		normalize_space(n->desc);
	}

	return n;
}


static xmlNodePtr 
find_ref(xmlNodePtr curr_node)
{
	xmlNodePtr n;
	char *name;
	char *tmp_name;
	struct element_node *c;

	name = (char *)xmlGetProp(curr_node, (xmlChar *)"name");
	dbg_printf("Trying to parse ref tag: %s\n", name);

	n = xmlDocGetRootElement(curr_node->doc);
	n = n->xmlChildrenNode;
	for (; n; n = n->next) {
		if (n->type != XML_ELEMENT_NODE)
			continue;
		if (strcasecmp((char *)n->name, "define"))
			continue;
		
		tmp_name = (char *)xmlGetProp(n, (xmlChar *)"name");
		if (!tmp_name)
			continue;
		if (strcmp(tmp_name, name))
			continue;

		break;
	}

	if (!n) {
		fprintf(stderr, "Error in RelaxNG schema!\n");
		fprintf(stderr, "Unterminated reference: %s\n",
			name);
		exit(1);
	}

	return n;
}


static int
find_optional_attributes(xmlNodePtr curr_node, int in_block,
			 struct element_node *curr_obj)
{
	xmlNodePtr node;
	struct attr_node *attr;
	struct attr_node *n;

	if (!curr_node || (curr_node->type == XML_ELEMENT_NODE &&
	    (curr_node->name && !strcasecmp((char *)curr_node->name, "element")))) {
		dbg_printf("No optionals; element tag\n");
		return 0;
	}

	dbg_printf("lookin for optionals\n");

	for (node = curr_node; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (!strcasecmp((char *)node->name, "ref")) {
			find_optional_attributes(find_ref(node)->xmlChildrenNode, 1, curr_obj);
			continue;
		}
		if (!strcasecmp((char *)node->name, "choice")) {
			find_optional_attributes(node->xmlChildrenNode, 1,
						 curr_obj);
			continue;
		}
		if (!strcasecmp((char *)node->name, "group")) {
			find_optional_attributes(node->xmlChildrenNode, 1,
						 curr_obj);
			continue;
		}
		if (!strcasecmp((char *)node->name, "optional")) {
			find_optional_attributes(node->xmlChildrenNode, 1,
						 curr_obj);
			continue;
		}

		if (!node->name || strcmp((char *)node->name,
			    "attribute")) {
			continue;
		}

		if (!in_block)
			continue;

		attr = get_attr(node);
		if (find_attr_byname(curr_obj->optional_attrs, attr->name) ||
		    find_attr_byname(curr_obj->required_attrs, attr->name)) {
			free(attr);
			continue;
		}

		dbg_printf("opt attr '%s'\n", attr->name);

		curr_obj->optional_attrs = g_list_insert_sorted(curr_obj->optional_attrs,
								attr,
								compare_attr);
	}
	return 0;
}


static int
find_required_attributes(xmlNodePtr curr_node,
			 struct element_node *curr_obj)
{
	xmlNodePtr node;
	struct attr_node *attr;

	dbg_printf("lookin for required\n");

	for (node = curr_node; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (xmlStrcmp(node->name, (xmlChar *)"attribute"))
			continue;

		attr = get_attr(node);
		if (find_attr_byname(curr_obj->optional_attrs, attr->name) ||
		    find_attr_byname(curr_obj->required_attrs, attr->name)) {
			free(attr);
			continue;
		}

		dbg_printf("req attr '%s'\n", attr->name);

		curr_obj->required_attrs = g_list_insert_sorted(curr_obj->required_attrs,
								attr,
								compare_attr);
	}
	return 0;
}


static struct element_node *
parse_element_tag(xmlNodePtr curr_node)
{
	struct element_node *obj;
	char *n, *desc;
	xmlAttrPtr ap;
	
	dbg_printf("Trying to parse element tag\n");
	n = (char *)xmlGetProp(curr_node, (xmlChar *)"name");

	if (!n)
		return NULL;

	obj = zalloc(sizeof(*obj));
	obj->this_node = curr_node;
	obj->name = n;
	obj->type = TYPE_ELEMENT;
	desc = xmlGetNsProp(curr_node, "description", RHA_URI);
	if (desc) {
		obj->desc = strdup(desc);
		normalize_space(obj->desc);
	}
	dbg_printf("New object class %s \n",obj->name);

	find_optional_attributes(curr_node->xmlChildrenNode, 0, obj);
	find_required_attributes(curr_node->xmlChildrenNode, obj);

	return obj;
}


static struct element_node *
parse_ref_tag(xmlNodePtr curr_node)
{
	struct element_node *obj;
	char *n, *desc;
	xmlAttrPtr ap;
	
	dbg_printf("Trying to parse ref tag\n");
	n = (char *)xmlGetProp(curr_node, (xmlChar *)"name");

	if (!n)
		return NULL;

	obj = zalloc(sizeof(*obj));
	obj->this_node = curr_node;
	obj->name = n;
	obj->type = TYPE_REF;

	return obj;
}


static struct element_node *
parse_define_tag(xmlNodePtr curr_node)
{
	struct element_node *obj;
	char *n, *desc;
	xmlAttrPtr ap;
	
	dbg_printf("Trying to parse ref tag\n");
	n = (char *)xmlGetProp(curr_node, (xmlChar *)"name");

	if (!n)
		return NULL;

	obj = zalloc(sizeof(*obj));
	obj->this_node = curr_node;
	obj->name = n;
	obj->type = TYPE_DEFINE;

	return obj;
}


int
find_objects(xmlNodePtr curr_node, struct element_node *parent, GList **tree, GList **objs)
{
	xmlNodePtr n;
	struct element_node *obj = NULL, *p;
	int ret = 0, x;

	if (!curr_node)
		/* no objects found */
		return 0;

	for (; curr_node; curr_node = curr_node->next) {
		if (curr_node->type != XML_ELEMENT_NODE)
			continue;
		if (!strcasecmp((char *)curr_node->name, "element")) {
			obj = parse_element_tag(curr_node);
			if (!obj) {
				continue;
			}

			obj->parent = parent;
			*tree = g_list_append(*tree, obj);
			*objs = g_list_insert_sorted(*objs, obj, compare_element);
			if (find_objects(curr_node->xmlChildrenNode, obj, &obj->children,
					 objs)) {
				ret = 1;
			}

		} else if (!strcasecmp((char *)curr_node->name, "ref")) {

			n = find_ref(curr_node);
			if (!n)
				continue;

			obj = parse_ref_tag(n);
			if (!obj) {
				continue;
			}

			obj->parent = parent;
			*tree = g_list_append(*tree, obj);

		} else if (!strcasecmp((char *)curr_node->name, "define")) {

			obj = parse_define_tag(curr_node);
			if (!obj) {
				continue;
			}

			obj->parent = parent;
			*tree = g_list_append(*tree, obj);

			if (find_objects(curr_node->xmlChildrenNode, obj, &obj->children,
					 objs)) {
				ret = 1;
			}

		} else {
			dbg_printf("Descend on %s\n", curr_node->name);
			if (find_objects(curr_node->xmlChildrenNode, parent, tree, objs)) {
				ret = 1;
			}
		}

	}

	/* Child objects were found */
	return ret;
}


void
display_req_attr(void *va, void *data)
{
	struct attr_node *a = va;
	FILE *fp = (FILE *)data;

 	fprintf(fp, "  <tr><td><b>%s</b></td><td>Required. %s</td></tr>\n", a->name, a->desc?a->desc:" ");
}

void
display_opt_attr(void *va, void *data)
{
	struct attr_node *a = va;
	FILE *fp = (FILE *)data;

 	fprintf(fp, "  <tr><td><b>%s</b></td><td>%s</td></tr>\n", a->name, a->desc?a->desc:" ");
}


void
_print_object(struct element_node *curr_obj, int tablevel, FILE *fp)
{
	int x;
	struct attr_node *a;

	if (!curr_obj)
		return;

	for (x = 0; x < tablevel; x++) {
		fprintf(fp, "&nbsp;&nbsp;&nbsp;&nbsp;");
		fprintf(fp, "&nbsp;&nbsp;&nbsp;&nbsp;");
	}

	if (curr_obj->type == TYPE_REF) {
		fprintf(fp, "<a href=\"#ref_%s\">%s</a><br/>\n", curr_obj->name, curr_obj->name);
	} else if (curr_obj->type == TYPE_DEFINE) {
		fprintf(fp, "<br/><a name=\"ref_%s\"/>%s definition<br/>\n", curr_obj->name, curr_obj->name);
	} else {
		fprintf(fp, "&lt;<a href=\"#tag_%s\">%s</a>%s%s&gt;<br/>\n", curr_obj->name, curr_obj->name, (curr_obj->required_attrs||curr_obj->optional_attrs)?" ...":"", curr_obj->children?"":"/");
	}
}


void
_print_object_close(struct element_node *curr_obj, int tablevel, FILE *fp)
{
	int x;
	struct attr_node *a;

	if (!curr_obj || curr_obj->type != TYPE_ELEMENT || !curr_obj->children)
		return;

	for (x = 0; x < tablevel; x++) {
		fprintf(fp, "&nbsp;&nbsp;&nbsp;&nbsp;");
		fprintf(fp, "&nbsp;&nbsp;&nbsp;&nbsp;");
	}

	fprintf(fp, "&lt;<a href=\"#tag_%s\">/%s</a>&gt;<br/>\n", curr_obj->name, curr_obj->name);
}


void _print_tree(GList *obj, int tablevel, FILE *fp);

void
_recurse_object(void *va, void *data)
{
	struct element_node *e = va;
	struct print_info *pi = (struct print_info *)data;

	_print_object(e, pi->tablevel, pi->fp);
	_print_tree(e->children, pi->tablevel+1, pi->fp);
	_print_object_close(e, pi->tablevel, pi->fp);
}

void
_print_tree(GList *obj, int tablevel, FILE *fp)
{
	struct print_info pi;

	pi.fp = fp;
	pi.tablevel = tablevel;

	g_list_foreach(obj, _recurse_object, &pi);
}


void
print_tree(FILE *fp, GList *root_obj)
{
	if (!root_obj)
		/* no objects found */
		return;
	if (!fp)
		fp = stdout;

	fprintf(fp, "<h2><a name=\"toc_tree_reference\"/>Tree Structure Reference</h2>\n");
	fprintf(fp, "<small><a href=\"#toc_tag_reference\">Jump to Tag Reference</a></small><br/><br/>\n");

	_print_tree(root_obj, 0, fp);
}


void
_print_tag_toc(void *c, void *a)
{
	struct element_node *curr_obj = (struct element_node *)c;
	FILE *fp = (FILE *)a;

	if (!curr_obj)
		return;
	if (!fp)
		fp = stdout;

	fprintf(fp, "<tr valign=\"top\"><td><a href=\"#tag_%s\"><b>%s</b></a></td><td>%s</td></tr>\n", curr_obj->name, curr_obj->name, curr_obj->desc);
}

void
_display_child(void *c, void *a)
{
	struct element_node *curr_obj = (struct element_node *)c;
	FILE *fp = (FILE *)a;

	if (!curr_obj)
		return;
	if (!fp)
		fp = stdout;

	if (curr_obj->type == TYPE_REF) {
		fprintf(fp, "<a href=\"#ref_%s\">%s</a> ", curr_obj->name, curr_obj->name, curr_obj->desc);
	} else {
		fprintf(fp, "<a href=\"#tag_%s\">%s</a> ", curr_obj->name, curr_obj->name, curr_obj->desc);
	}
}

void
_print_tag_reference(void *c, void *a)
{
	struct element_node *curr_obj = (struct element_node *)c;
	FILE *fp = (FILE *)a;

	if (!curr_obj)
		return;
	if (!fp)
		fp = stdout;

	fprintf(fp, "<h3><a name=\"tag_%s\"/>", curr_obj->name);
	fprintf(fp, "%s</h3>\n", curr_obj->name);
	if (curr_obj->desc)
		fprintf(fp, "%s<br/>\n", curr_obj->desc);
	fprintf(fp, "<br/>\n");

	if (curr_obj->required_attrs || curr_obj->optional_attrs) {
		fprintf(fp, "<table><tr valign=\"top\"><td>Attribute</td><td>Description</td></tr>\n", curr_obj->name);

		g_list_foreach(curr_obj->required_attrs, display_req_attr, (void *)fp);
		g_list_foreach(curr_obj->optional_attrs, display_opt_attr, (void *)fp);
		fprintf(fp, "</table><br/>\n");
	}


	if (curr_obj->children) {
		fprintf(fp, "Children: ");
		g_list_foreach(curr_obj->children, _display_child, (void *)fp);
		fprintf(fp, "<br/>");
	}

	fprintf(fp, "<a href=\"#\" onClick=\"history.go(-1)\" />Back</a> | ");
	fprintf(fp, "<a href=\"#toc_tag_reference\">Contents</a>\n");
}


void
print_tag_reference(FILE *fp, GList *root_obj)
{
	if (!fp)
		fp = stdout;
	if (!root_obj)
		/* no objects found */
		return;

	fprintf(fp, "<h2>Tag Reference<a name=\"toc_tag_reference\"/></h2>");
	fprintf(fp, "<small><a href=\"#toc_tree_reference\">Jump to Tree Structure Reference</a></small><br/><br/>\n");
	fprintf(fp, "<table>\n");
	g_list_foreach(root_obj, _print_tag_toc, (void *)fp);
	fprintf(fp, "</table>\n");
	fprintf(fp, "<hr/>\n");
	g_list_foreach(root_obj, _print_tag_reference, (void *)fp);
}
