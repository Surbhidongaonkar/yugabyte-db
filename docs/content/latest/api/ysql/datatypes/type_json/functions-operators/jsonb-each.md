---
title: jsonb_each()
linkTitle: jsonb_each()
summary: jsonb_each() and json_each()
headerTitle: jsonb_each() and json_each()
description: The functions in this pair require that the supplied JSON value is an object. They return a row set with columns "key" and "value"
menu:
  latest:
    identifier: jsonb-each
    parent: functions-operators
    weight: 110
isTocNested: true
showAsideToc: true
---

**Purpose:** create a row set with columns _"key"_ (as a SQL `text`) and _"value"_ (as a SQL `jsonb`) from a JSON _object_.

**Signature** for the `jsonb` variant:

```
input value:       jsonb
return value:      SETOF (text, jsonb)
```

Use this _ysqlsh_ script to create the required type `t` and then to execute the `assert`.

```postgresql
create type t as (k text, v jsonb);

do $body$
declare
  object constant jsonb :=
    '{"a": 17, "b": "dog", "c": true, "d": {"a": 17, "b": "cat"}}';

  k_v_pairs t[] := null;
  expected_k_v_pairs constant t[] := 
    array[
      ('a', '17'::jsonb),
      ('b', '"dog"'::jsonb),
      ('c', 'true'::jsonb),
      ('d', '{"a": 17, "b": "cat"}'::jsonb)
    ];

  k_v_pair t;
  n int := 0;
begin
  for k_v_pair in (
    select key, value from jsonb_each(object)
    )
  loop
    n := n + 1;
    k_v_pairs[n] := k_v_pair;
  end loop;

  assert
    k_v_pairs = expected_k_v_pairs,
  'unexpected';
end;
$body$;
```
