---
title: Indexes and check constraints
linkTitle: Indexes and check constraints
summary: Create indexes and check constraints on JSON columns
headerTitle: Create indexes and check constraints on JSON columns
description: Create indexes and check constraints on JSON columns
menu:
  latest:
    identifier: create-indexes-check-constraints
    parent: api-ysql-datatypes-json
    weight: 50
isTocNested: true
showAsideToc: true
---

Often, when  JSON documents are inserted into a table, the table will have just a self-populating surrogate primary key column and a value column, like `doc`, of datatype `jsonb`. Choosing `jsonb` allows the use of a broader range of operators and functions, and allows these to execute more efficiently, than does choosing `json`.

It's most likely that each document will be a JSON _object_ and that all will conform to the same structure definition. (The structure can be defined formally, and externally, by a so-called "[JSON schema](https://json-schema.org)".) In other words, each _object_ will have the same set of possible key names (but some might be missing) and the same JSON datatype for the value for each key. And when a datatype is compound, the same notion of common structure definition will apply, extending the notion recursively to arbitrary depth. Here is an example. To reduce clutter, the primary key is not defined to be self-populating. 

```postgresql
create table books(k int primary key, doc jsonb not null);

insert into books(k, doc) values
  (1,
  '{ "ISBN"    : 4582546494267,
     "title"   : "Macbeth", 
     "author"  : {"given_name": "William", "family_name": "Shakespeare"},
     "year"    : 1623}'),

  (2,
  '{ "ISBN"    : 8760835734528,
     "title"   : "Hamlet",
     "author"  : {"given_name": "William", "family_name": "Shakespeare"},
     "year"    : 1603,
     "editors" : ["Lysa", "Elizabeth"] }'),

  (3,
  '{ "ISBN"    : 7658956876542,
     "title"   : "Oliver Twist",
     "author"  : {"given_name": "Charles", "family_name": "Dickens"},
     "year"    : 1838,
     "genre"   : "novel",
     "editors" : ["Mark", "Tony", "Britney"] }'),
  (4,
  '{ "ISBN"    : 9874563896457,
     "title"   : "Great Expectations",
     "author"  : {"family_name": "Dickens"},
     "year"    : 1950,
     "genre"   : "novel",
     "editors" : ["Robert", "John", "Melisa", "Elizabeth"] }'),

  (5,
  '{ "ISBN"    : 8647295405123,
     "title"   : "A Brief History of Time",
     "author"  : {"given_name": "Stephen", "family_name": "Hawking"},
     "year"    : 1988,
     "genre"   : "science",
     "editors" : ["Melisa", "Mark", "John", "Fred", "Jane"] }'),

  (6,
  '{
    "ISBN"     : 6563973589123,
    "year"     : 1989,
    "genre"    : "novel",
    "title"    : "Joy Luck Club",
    "author"   : {"given_name": "Amy", "family_name": "Tan"},
    "editors"  : ["Ruilin", "Aiping"]}');
```

Some of the rows have some of the keys missing. But the row with `k = 6` has every key.

You will probably want at least to know if your corpus contains a non-conformant document and, in some cases, you will want to disallow non-conformant documents. You might want to insist that the ISBN is always defined and is a positive 13-digit number.

You will almost certainly want to retrieve documents, not simply by providing the key, but rather by using predicates on their content—the primitive values that they contain. You will probably want, also, to project out values of interest.

For example, you might want to see the title and author of books whose publication year is later than 1850.

Of course, then, you will want these queries to be supported by indexes. (The alternative – a table scan over a huge corpus where each document is analyzed on the fly to evaluate the selection predicates is simply unworkable.)

### Check constraints on _jsonb_ columns

Here's how to insist that each JSON document is an _object_:
```postgresql
alter table books 
add constraint books_doc_is_object
check (jsonb_typeof(doc) = 'object');
```
Here's how to insist that the ISBN is always defined and is a positive 13-digit number:
```postgresql
alter table books 
add constraint books_isbn_is_positive_13_digit_number 
check (
  (doc->'ISBN') is not null
    and
  jsonb_typeof(doc->'ISBN') = 'number'
     and
  (doc->>'ISBN')::bigint > 0
    and
  length(((doc->>'ISBN')::bigint)::text) = 13
);
```
Notice that if the key _"ISBN"_ is missing altogether, then the expression `doc->'ISBN'` will yield a genuine SQL `null`. But the producer of the document might have decided to represent *"No information is available about this book's ISBN"* with the special JSON value _null_ for the key _"ISBN"_. (Recall that this special value has its own data type.) This is why it's insufficient to test just that `jsonb_typeof()`, (described [here](../functions-operators/jsonb-typeof/), yields _number_ (and therefore not _null_) for the key  _"ISBN"_ and why the separate `is not null` test is done as well.

The high-level point is that YSQL allows you to express a constraint using any expression that can be evaluated by referencing values from a single row. The expression can include a PL/pgSQL function. This allows a constraint to be implemented to insist that the keys in the JSON _object_ are from a known list:

```postgresql
create function top_level_keys_ok(json_obj in jsonb)
  returns boolean
  immutable
  language plpgsql
as
$body$
declare
  key text;
  legal_keys constant varchar(10)[] := array[
    'ISBN', 'title', 'year', 'genre', 'author', 'editors'];
begin
  for key in (
    select
    jsonb_object_keys(json_obj)
    )
  loop
    if not (key = any (legal_keys)) then
      return false;
    end if;
  end loop;
  return true;
end;
$body$;

alter table books 
add constraint books_doc_keys_OK
check (top_level_keys_ok(doc));
```

The `jsonb_object_keys()` function is described [here](http://localhost:1313/latest/api/ysql/datatypes/type_json/functions-operators/jsonb-object-keys/).

### Indexes on _jsonb_ columns

Proper practice requires that when a table has a surrogate primary key, it must also have a unique, not null, business key. The obvious candidate for the `books` table is the value for the _"ISBN"_ key. The _"not null"_ rule is already enforced by the `books_isbn_is_positive_13_digit_number` constraint. Uniqueness is enforced in the obvious way:


```postgresql
create unique index books_isbn_unq
on books((doc->>'ISBN') hash);
```
You might want to support range queries that reference the value for the _"year"_ key like this:
```postgresql
select
  (doc->>'ISBN')::bigint as year,
  doc->>'title'          as title,
  (doc->>'year')::int    as year
from books
where (doc->>'year')::int > 1850
order by 3;
```

You'll probably want to support this with an index. And if you realize that the publication year is unknown for a substantial proportion of the books, you will probably want to take advantage of a partial index, thus:

```postgresql
create index books_year on books ((doc->>'year') asc)
where doc->>'year' is not null;
```
