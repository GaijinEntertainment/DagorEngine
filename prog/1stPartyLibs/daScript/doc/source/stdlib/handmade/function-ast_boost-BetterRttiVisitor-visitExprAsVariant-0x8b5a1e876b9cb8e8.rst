Visitor override that replaces ``expr as Type`` on ``Expression`` subclasses with an RTTI-checked cast via ``__rtti``, panicking on mismatch.
