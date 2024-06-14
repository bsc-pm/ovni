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
#include "pv/prv.h"
#include "pv/pvt.h"
#include "thread.h"
#include "uthash.h"
#include <errno.h>

struct mark_label {
	int64_t value;
	char label[MAX_PCF_LABEL];
	UT_hash_handle hh; /* Indexed by value */
};

struct mark_type {
	long type;
	long index; /* From 0 to ntypes - 1 */
	enum chan_type ctype;
	struct mark_label *labels; /* Hash table of labels */
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
find_label(struct mark_type *t, int64_t value)
{
	struct mark_label *l;
	HASH_FIND(hh, t->labels, &value, sizeof(value), l);
	return l;
}

static int
add_label(struct mark_type *t, int64_t value, const char *label)
{
	struct mark_label *l = find_label(t, value);

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

	HASH_ADD(hh, t->labels, value, sizeof(value), l);

	return 0;
}

static int
parse_labels(struct mark_type *t, JSON_Object *labels)
{
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

		if (add_label(t, value, label) != 0) {
			err("add_label() failed");
			return -1;
		}
	}

	return 0;
}

static struct mark_type *
find_mark_type(struct ovni_mark_emu *m, long type)
{
	struct mark_type *t;
	HASH_FIND_LONG(m->types, &type, t);
	return t;
}

static struct mark_type *
create_mark_type(struct ovni_mark_emu *m, long type, const char *chan_type, const char *title)
{
	struct mark_type *t = find_mark_type(m, type);

	if (t != NULL) {
		err("mark type %d already defined", type);
		return NULL;
	}

	t = calloc(1, sizeof(*t));
	if (t == NULL) {
		err("calloc failed:");
		return NULL;
	}

	t->type = type;
	t->index = m->ntypes;

	int len = snprintf(t->title, MAX_PCF_LABEL, "%s", title);
	if (len >= MAX_PCF_LABEL) {
		err("mark title too long: %s", title);
		return NULL;
	}

	if (strcmp(chan_type, "single") == 0) {
		t->ctype = CHAN_SINGLE;
	} else if (strcmp(chan_type, "stack") == 0) {
		t->ctype = CHAN_STACK;
	} else {
		err("chan_type %s not understood", chan_type);
		return NULL;
	}

	HASH_ADD_LONG(m->types, type, t);
	m->ntypes++;

	return t;
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

	struct mark_type *t = find_mark_type(m, type);
	if (t == NULL) {
		t = create_mark_type(m, type, chan_type, title);
		if (t == NULL) {
			err("cannot create mark type");
			return -1;
		}
	} else {
		/* It may already exist as defined by other threads, so ensure
		 * they have the same title. */
		if (strcmp(t->title, title) != 0) {
			err("mark with type %ld already registered with another title", type);
			err(" old: %s", t->title);
			err(" new: %s", title);
			return -1;
		}
	}

	/* Now populate the mark type with all value labels */

	if (parse_labels(t, labels) != 0) {
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

static int
create_thread_chan(struct ovni_mark_emu *m, struct bay *bay, struct thread *th)
{
	struct ovni_thread *oth = EXT(th, 'O');
	struct ovni_mark_thread *t = &oth->mark;

	/* Create as many channels as required */
	t->channels = calloc(m->ntypes, sizeof(struct chan));
	if (t->channels == NULL) {
		err("calloc failed:");
		return -1;
	}

	t->nchannels = m->ntypes;

	struct mark_type *type;
	for (type = m->types; type; type = type->hh.next) {
		/* TODO: We may use a vector of thread channels in every type to
		 * avoid the double hash access in events */
		long i = type->index;
		struct chan *ch = &t->channels[i];
		chan_init(ch, type->ctype, "thread%ld.mark%ld",
				th->gindex, type->type);

		if (bay_register(bay, ch) != 0) {
			err("bay_register failed");
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

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		if (create_thread_chan(memu, &emu->bay, th) != 0) {
			err("create_thread_chan failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *sth, struct prv *prv)
{
	struct ovni_emu *oemu = EXT(emu, 'O');
	struct ovni_mark_emu *memu = &oemu->mark;
	struct ovni_thread *oth = EXT(sth, 'O');
	struct ovni_mark_thread *mth = &oth->mark;

	for (struct mark_type *type = memu->types; type; type = type->hh.next) {
		/* TODO: We may use a vector of thread channels in every type to
		 * avoid the double hash access in events */
		long i = type->index;
		struct chan *ch = &mth->channels[i];
		long row = sth->gindex;
		long flags = 0;
		long prvtype = type->type + PRV_OVNI_MARK;
		if (prv_register(prv, row, prvtype, &emu->bay, ch, flags)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_thread(struct emu *emu)
{
	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "thread");
	if (pvt == NULL) {
		err("cannot find thread pvt");
		return -1;
	}

	/* Connect thread channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct thread *t = emu->system.threads; t; t = t->gnext) {
		if (connect_thread_prv(emu, t, prv) != 0) {
			err("connect_thread_prv failed");
			return -1;
		}
	}

	/* TODO: Init thread PCF */
	return 0;
}

/* Connect the channels to the output PVTs */
int
mark_connect(struct emu *emu)
{
	if (connect_thread(emu) != 0) {
		err("connect_thread() failed");
		return -1;
	}

	/* TODO: Connect CPU */

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

	struct mark_type *mc = find_mark_type(memu, type);

	if (mc == NULL) {
		err("cannot find mark with type %ld", type);
		return -1;
	}

	long index = mc->index;
	struct ovni_thread *oth = EXT(emu->thread, 'O');
	struct ovni_mark_thread *mth = &oth->mark;

	struct chan *ch = &mth->channels[index];

	switch (emu->ev->v) {
		case '[':
			return chan_push(ch, value_int64(value));
		case ']':
			return chan_pop(ch, value_int64(value));
		default:
			err("unknown mark event value %c", emu->ev->v);
			return -1;
	}
}
