// Copyright (c) 2018, Hunt Zhan
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "cpp11/sam.h"

namespace py = pybind11;

PYBIND11_MODULE(_sam_impl, m) {
    m.doc() = R"pbdoc(
      TODO
    )pbdoc";

    // Just return the pointer.
    py::class_<sam::SamState>(m, "SamState");

    py::class_<sam::SamStateOpt>(m, "SamStateOpt")
        // Build.
        .def(py::init<>())
        .def("online", &sam::SamStateOpt::OnlineConstructSymbol,
             py::arg("symbol"), py::arg("maxlen_limit") = -1)
        .def("online", &sam::SamStateOpt::OnlineConstructFactor,
             py::arg("factor"), py::arg("maxlen_limit") = -1)
        .def("finalize", &sam::SamStateOpt::Finalize)
        // Inference.
        .def("occur_count", &sam::SamStateOpt::OccurCount)
        .def("occur_degree", &sam::SamStateOpt::OccurDegree,
             py::arg("factor"), py::arg("cap") = 1000.0)
        .def("out_count", &sam::SamStateOpt::OutCount)
        .def("out_degree", &sam::SamStateOpt::OutDegree);


#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
