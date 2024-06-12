/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>
#include "instr.h"
#include "ovni.h"

/* Check stream attribute setting and getting. */
int
main(void)
{
	instr_start(0, 1);

	ovni_attr_set_double("ovni.test.double", 123.5);
	if (ovni_attr_get_double("ovni.test.double") != 123.5)
		die("mismatch double");

	ovni_attr_set_boolean("ovni.test.boolean", 1);
	if (!ovni_attr_get_boolean("ovni.test.boolean"))
		die("mismatch boolean");

	ovni_attr_set_str("ovni.test.str", "foo");
	const char *str = ovni_attr_get_str("ovni.test.str");
	if (str == NULL || strcmp(str, "foo") != 0)
		die("mismatch string");

	const char *json = "{\"foo\":42}";
	ovni_attr_set_json("nosv.test.json", json);

	char *json2 = ovni_attr_get_json("nosv.test.json");
	if (strcmp(json, json2) != 0)
		die("mismatch json: in '%s' out '%s'", json, json2);

	if (!ovni_attr_has("nosv.test.json.foo"))
		die("missing attribute");

	if (ovni_attr_get_double("nosv.test.json.foo") != 42.0)
		die("mismatch double");

	free(json2);

	instr_end();

	return 0;
}
