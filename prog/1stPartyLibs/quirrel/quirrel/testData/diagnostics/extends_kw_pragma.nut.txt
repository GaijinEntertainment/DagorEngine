class B {}

class D1 (B) { }
class D2 extends B { } // FP 1

#forbid-extends-keyword

class D3 (B) { }

#allow-extends-keyword

class D4 extends B { } // FP 2

#forbid-extends-keyword

class D5 extends B { } // EXPECTED