This annotation reloads nodes on the fly after script reloading.
It is designed for functions accepting temp value or ref to :ref:`daBfg::NodeHandle <handle-daBfg-NodeHandle>`, acquired from ecs system.
Annotation accepts three parameters:

+------+----------------------------------------------------------------+
+name  +string name of registred node inside annotated function         +
+------+----------------------------------------------------------------+
+entity+string name of entity in which node is stored                   +
+------+----------------------------------------------------------------+
+handle+string name of component of type :cpp:class:`dabfg::NodeHandle` +
+------+----------------------------------------------------------------+
