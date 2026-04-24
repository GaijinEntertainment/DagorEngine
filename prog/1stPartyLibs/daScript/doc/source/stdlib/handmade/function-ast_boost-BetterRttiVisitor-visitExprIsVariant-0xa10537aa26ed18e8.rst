Visitor override that replaces ``expr is Type`` on ``Expression`` subclasses with an ``__rtti`` string comparison, returning ``true`` if the runtime type matches.
