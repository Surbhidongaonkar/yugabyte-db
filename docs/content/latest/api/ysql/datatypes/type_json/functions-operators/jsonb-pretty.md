---
title: jsonb_pretty()
linkTitle: jsonb_pretty() 
summary: jsonb_pretty() 
description: jsonb_pretty()
menu:
  latest:
    identifier: jsonb-pretty
    parent: functions-operators
    weight: 200
isTocNested: true
showAsideToc: true
---

**Purpose:** format the text representation of the JSON value that the input `jsonb` actual argument represents, using whitespace, to make it maximally easily human readable.

**Signature:**

```
input value:       jsonb
return value:      text
```

**Notes:** `jsonb_pretty()` has no variant for a plain `json` value. However, you can trivially typecast a `json` value to a `jsonb` value.

Because, in the main, JSON is mechanically generated and mechanically consumed, it's unlikely that you'll use `jsonb_pretty()` in final production code. However, because JSON isn't self-describing, and because external documentation of a corpus's data-representation intent isn't always available, developers often need to study a representative set of extant JSON values and deduce the intended rules of composition. This task is made hugely easier when JSON values are formatted in a standard, well designed, way.

The pretty format makes conventional, and liberal, use of newlines and spaces, thus:

```postgresql
do $body$
declare
  orig_text constant text := '
    {
      "a": 1,
      "b": {"x": 1, "y": 19},
      "c": true
    }';

  expected_pretty_text text :=
'{
    "a": 1,
    "b": {
        "x": 1,
        "y": 19
    },
    "c": true
}';

  j constant jsonb := orig_text::jsonb;
  pretty_text constant text := jsonb_pretty(j);
begin
  assert
    pretty_text = expected_pretty_text,
 'unexpected';
end;
$body$;
```

You might prefer simply to use the `::text` typecast to visualize a small `jsonb` value.
