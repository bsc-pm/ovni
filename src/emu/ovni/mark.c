#include "mark.h"

#include "chan.h"
#include "emu.h"
#include "emu_ev.h"
#include "emu_prv.h"
#include "inttypes.h"
#include "ovni.h"
#include "ovni_priv.h"
#include "parson.h"
#include "pv/pcf.h"
#include "thread.h"
#include "uthash.h"
#include <errno.h>

struct mark_label {
	int64_t value;
	char label[MAX_PCF_LABEL];
	UT_hash_handle hh; /* Indexed by value */
};

struct mark_chan {
	long type;
	struct mark_label *labels; /* Hash table of labels */
	struct chan ch;
	char title[MAX_PCF_LABEL];
	UT_hash_handle hh; /* Indexed by type */
};

static int
parse_number(const char *str, int64_t *result)
{
	errno = 0;
	char *endptr = NULL;
	int64_t n = strtoll(str, &endptr, 10);

	if (errno != 0 || endptr == str || endptr[0] != '\0') {
		err("failed to parse number: %s", str);
		return -1;
	}

	*result = n;
	return 0;
}

static struct mark_label *
find_label(struct mark_chan *c, int64_t value)
{
	struct mark_label *l;
	HASH_FIND(hh, c->labels, &value, sizeof(value), l);
	return l;
}

static int
add_label(struct mark_chan *c, int64_t value, const char *label)
{
	struct mark_label *l = find_label(c, value);

	if (l != NULL) {
		if (strcmp(l->label, label) == 0) {
			/* Already exists with the same label, all good */
			return 0;
		} else {
			err("mark value %" PRIi64 " already defined with label %s", value, l->label);
			return -1;
		}
	}

	l = calloc(1, sizeof(*l));
	if (l == NULL) {
		err("calloc failed:");
		return -1;
	}

	l->value = value;

	int len = snprintf(l->label, MAX_PCF_LABEL, "%s", label);
	if (len >= MAX_PCF_LABEL) {
		err("mark label too long: %s", label);
		return -1;
	}

	HASH_ADD(hh, c->labels, value, sizeof(value), l);

	return 0;
}

static int
parse_labels(struct mark_chan *c, JSON_Object *labels)
{
	UNUSED(c);
	UNUSED(labels);

	/* It may happen that we call this function several times with
	 * overlapping subsets of values. The only restriction is that we don't
	 * define two values with different label. */

	size_t n = json_object_get_count(labels);
	for (size_t i = 0; i < n; i++) {
		const char *valuestr = json_object_get_name(labels, i);
		if (valuestr == NULL) {
			err("json_object_get_name failed");
			return -1;
		}

		int64_t value;
		if (parse_number(valuestr, &value) != 0) {
			err("parse_number failed");
			return -1;
		}

		JSON_Value *labelval = json_object_get_value_at(labels, i);
		if (labelval == NULL) {
			err("json_object_get_value_at failed");
			return -1;
		}

		const char *label = json_value_get_string(labelval);
		if (label == NULL) {
			err("json_value_get_string() for label failed");
			return -1;
		}

		if (add_label(c, value, label) != 0) {
			err("add_label() failed");
			return -1;
		}
	}

	return 0;
}

static struct mark_chan *
find_mark_chan(struct ovni_mark_emu *m, long type)
{
	struct mark_chan *c;
	HASH_FIND_LONG(m->chan, &type, c);
	return c;
}

static struct mark_chan *
create_mark_chan(struct ovni_mark_emu *m, long type, const char *chan_type, const char *title)
{
	struct mark_chan *c = find_mark_chan(m, type);

	if (c != NULL) {
		err("mark type %d already defined", type);
		return NULL;
	}

	c = calloc(1, sizeof(*c));
	if (c == NULL) {
		err("calloc failed:");
		return NULL;
	}

	c->type = type;

	int len = snprintf(c->title, MAX_PCF_LABEL, "%s", title);
	if (len >= MAX_PCF_LABEL) {
		err("mark title too long: %s", title);
		return NULL;
	}

	enum chan_type ctype;
	if (strcmp(chan_type, "single") == 0) {
		ctype = CHAN_SINGLE;
	} else if (strcmp(chan_type, "stack") == 0) {
		ctype = CHAN_STACK;
	} else {
		err("chan_type %s not understood", chan_type);
		return NULL;
	}

	chan_init(&c->ch, ctype, "mark%ld", type);

	HASH_ADD_LONG(m->chan, type, c);

	return c;
}

static int
parse_mark(struct ovni_mark_emu *m, const char *typestr, JSON_Value *markval)
{
	errno = 0;
	char *endptr = NULL;
	long type = strtol(typestr, &endptr, 10);

	if (errno != 0 || endptr == typestr || endptr[0] != '\0') {
		err("failed to parse type number: %s", typestr);
		return -1;
	}

	if (type < 0 || type >= 100) {
		err("mark type should be in [0, 100) range: %ld", type);
		return -1;
	}

	JSON_Object *mark = json_value_get_object(markval);
	if (mark == NULL) {
		err("json_value_get_object() failed");
		return -1;
	}

	const char *title = json_object_get_string(mark, "title");
	if (title == NULL) {
		err("json_object_get_string() for title failed");
		return -1;
	}

	const char *chan_type = json_object_get_string(mark, "chan_type");
	if (chan_type == NULL) {
		err("json_object_get_string() for chan_type failed");
		return -1;
	}

	JSON_Object *labels = json_object_get_object(mark, "labels");
	if (labels == NULL) {
		err("json_object_get_object() for labels failed");
		return -1;
	}

	struct mark_chan *c = find_mark_chan(m, type);
	if (c == NULL) {
		c = create_mark_chan(m, type, chan_type, title);
		if (c == NULL) {
			err("cannot create mark chan");
			return -1;
		}
	} else {
		/* It may already exist as defined by other threads, so ensure
		 * they have the same title. */
		if (strcmp(c->title, title) != 0) {
			err("mark with type %ld already registered with another title", type);
			err(" old: %s", c->title);
			err(" new: %s", title);
			return -1;
		}
	}

	/* Now populate the mark channel with all value labels */

	if (parse_labels(c, labels) != 0) {
		err("cannot parse labels");
		return -1;
	}

	return 0;
}

static int
scan_thread(struct ovni_mark_emu *memu, struct thread *t)
{
	JSON_Object *obj = json_object_dotget_object(t->meta, "ovni.mark");

	/* No marks in this thread */
	if (obj == NULL)
		return 0;

	memu->has_marks = 1;

	size_t n = json_object_get_count(obj);
	for (size_t i = 0; i < n; i++) {
		const char *typestr = json_object_get_name(obj, i);
		if (typestr == NULL) {
			err("json_object_get_name failed");
			return -1;
		}
		JSON_Value *markval = json_object_get_value_at(obj, i);
		if (markval == NULL) {
			err("json_object_get_value_at failed");
			return -1;
		}

		if (parse_mark(memu, typestr, markval) != 0) {
			err("cannot parse mark");
			return -1;
		}
	}

	return 0;
}

/* Scans streams for marks and creates the mark channels */
int
mark_create(struct emu *emu)
{
	struct ovni_emu *oemu = EXT(emu, 'O');
	struct ovni_mark_emu *memu = &oemu->mark;

	memset(memu, 0, sizeof(*memu));

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		if (scan_thread(memu, th) != 0) {
			err("scan_thread failed");
			return -1;
		}
	}

	return 0;
}

/* Connect the channels to the output PVTs */
int
mark_connect(struct emu *emu)
{
	UNUSED(emu);

	/* TODO: Implement */

	return 0;
}

int
mark_event(struct emu *emu)
{
	if (emu->ev->payload_size != 8 + 4) {
		err("unexpected payload size %d", emu->ev->payload_size);
		return -1;
	}

	struct ovni_emu *oemu = EXT(emu, 'O');
	struct ovni_mark_emu *memu = &oemu->mark;

	int64_t value = emu->ev->payload->i64[0];
	long type = (long) emu->ev->payload->i32[2]; /* always fits */

	struct mark_chan *mc = find_mark_chan(memu, type);

	if (mc == NULL) {
		err("cannot find mark with type %ld", type);
		return -1;
	}

	/* TODO: Remove stub */
	//struct chan *ch = &mc->ch;

	switch (emu->ev->v) {
		case '[':
			//return chan_push(ch, value_int64(value));
			info("chan_push(ch, value_int64(%ld))", value);
			return 0;
		case ']':
			//return chan_pop(ch, value_int64(value));
			info("chan_pop(ch, value_int64(%ld))", value);
			return 0;
		default:
			err("unknown mark event value %c", emu->ev->v);
			return -1;
	}
}
