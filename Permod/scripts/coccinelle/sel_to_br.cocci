virtual context
virtual patch

//
// Pattern 1-A
//
@ depends on context && !patch @
expression x, y;
@@

* return x ? -EACCES : y;

@ depends on !context && patch @
expression x, y;
@@

- return x ? -EACCES : y;
+ { if (x) return -EACCES; else return y; }


//
// Pattern 1-B
//
@ depends on context && !patch @
expression x, y;
@@

* return x ? y : -EACCES;

@ depends on !context && patch @
expression x, y;
@@

- return x ? y : -EACCES;
+ { if (x) return y; else return -EACCES; }


//
// Pattern 2-A
//
@ depends on context && !patch @
expression v, x, y;
@@

* v = x ? -EACCES : y;

@ depends on !context && patch @
expression v, x, y;
@@

- v = x ? -EACCES : y;
+ { if (x) v = -EACCES; else v = y; }


//
// Pattern 2-B
//
@ depends on context && !patch @
expression v, x, y;
@@

* v = x ? y : -EACCES;

@ depends on !context && patch @
expression v, x, y;
@@

- v = x ? y : -EACCES;
+ { if (x) v = -EACCES; else v = y; }

