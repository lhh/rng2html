#ifndef _CLUSTER_H
#define _CLUSTER_H

#define RHA_URI "http://redhat.com/~pkennedy/annotation_namespace/cluster_conf_annot_namespace"

struct attr_node {
	char *name;
	char *desc;
	xmlNodePtr this_node;
};

struct element_node {
	struct element_node *parent;
	GList *children;
	GList *required_attrs;
	GList *optional_attrs;
	char *name;
	char *desc;
	int type;
#define TYPE_ELEMENT	0
#define TYPE_REF	1
#define TYPE_DEFINE	2
	xmlNodePtr this_node;
};

int find_objects(xmlNodePtr curr_node, struct element_node *parent, GList **tree, GList **objs);
void print_objects(FILE *outfile, GList *root_obj);
void print_tree(FILE *outfile, GList *root_obj);

#endif

