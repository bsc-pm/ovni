#include "clkoff.h"

#include "common.h"
#include <errno.h>

static struct clkoff_entry *
cfind(struct clkoff *off, const char *name)
{
	struct clkoff_entry *entry = NULL;

	HASH_FIND_STR(off->entries, name, entry);
	return entry;
}

static int
cadd(struct clkoff *table, struct clkoff_entry e)
{
	if (cfind(table, e.name) != NULL) {
		err("entry with name '%s' already exists\n", e.name);
		return -1;
	}

	struct clkoff_entry *entry = calloc(1, sizeof(struct clkoff_entry));
	if (entry == NULL) {
		err("calloc failed\n");
		return -1;
	}

	memcpy(entry, &e, sizeof(*entry));
	
	HASH_ADD_STR(table->entries, name, entry);
	table->nentries++;

	return 0;
}

static int
cparse(struct clkoff *table, FILE *file)
{
	/* Ignore header line */
	char buf[1024];
	if (fgets(buf, 1024, file) == NULL) {
		err("missing header line in clock offset\n");
		return -1;
	}

	for (int line = 1; ; line++) {
		errno = 0;
		struct clkoff_entry e = { 0 };

		if (fgets(buf, 1024, file) == NULL)
			break;

		/* Empty line */
		if (buf[0] == '\n')
			continue;

		int ret = sscanf(buf, "%ld %s %lf %lf %lf",
				&e.index, e.name,
				&e.median, &e.mean, &e.stdev);

		if (ret == EOF && errno == 0)
			break;

		if (ret == EOF && errno != 0) {
			err("fscanf() failed in line %d\n", line);
			return -1;
		}

		if (ret != 5) {
			err("fscanf() read %d instead of 5 fields in line %d\n",
					ret, line);
			return -1;
		}

		if (cadd(table, e) != 0) {
			err("cannot add entry of line %d\n",
					line);
			return -1;
		}
	}

	return 0;
}

static int
cindex(struct clkoff *table)
{
	if (table->nentries < 1) {
		err("no entries\n");
		return -1;
	}

	table->index = calloc(table->nentries, sizeof(struct clkoff_entry *));

	if (table->index == NULL) {
		err("calloc failed\n");
		return -1;
	}

	struct clkoff_entry *e;
	int i = 0;
	for (e = table->entries; e; e = e->hh.next)
		table->index[i++] = e;

	return 0;
}

void
clkoff_init(struct clkoff *table)
{
	memset(table, 0, sizeof(struct clkoff));
}

int
clkoff_load(struct clkoff *table, FILE *file)
{
	if (cparse(table, file) != 0) {
		err("clkoff_load: failed parsing clock table\n");
		return -1;
	}

	/* Create index array */
	if (cindex(table) != 0) {
		err("clkoff_load: failed indexing table\n");
		return -1;
	}

	return 0;
}

int
clkoff_count(struct clkoff *table)
{
	return table->nentries;
}

struct clkoff_entry *
clkoff_get(struct clkoff *table, int i)
{
	return table->index[i];
}
