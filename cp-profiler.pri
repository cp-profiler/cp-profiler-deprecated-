RESOURCES += $$PWD/cp-profiler.qrc

SOURCES += \
    $$PWD/globalhelper.cpp \
    $$PWD/gistmainwindow.cpp \
    $$PWD/heap.cpp \
    $$PWD/nodewidget.cpp \
    $$PWD/drawingcursor.cpp \
    $$PWD/treecanvas.cpp \
    $$PWD/visualnode.cpp \
    $$PWD/nodestats.cpp \
    $$PWD/preferences.cpp \
    $$PWD/spacenode.cpp \
    $$PWD/node.cpp \
    $$PWD/data.cpp \
    $$PWD/nodetree.cpp \
    $$PWD/cmp_tree_dialog.cpp \
    $$PWD/receiverthread.cpp \
    $$PWD/treebuilder.cpp \
    $$PWD/readingQueue.cpp \
    $$PWD/treecomparison.cpp \
    $$PWD/nogood_dialog.cpp \
    $$PWD/node_info_dialog.cpp \
    $$PWD/cpprofiler\pixeltree\pixelImage.cpp \
    $$PWD/maybeCaller.cpp \
    $$PWD/message.pb.cpp \
    $$PWD/profiler-conductor.cpp \
    $$PWD/profiler-tcp-server.cpp \
    $$PWD/ml-stats.cpp \
    $$PWD/execution.cpp \
    $$PWD/tree_utils.cpp \
    $$PWD/cpprofiler\analysis\shape_aggregation.cpp \
    $$PWD/cpprofiler\analysis\backjumps.cpp \
    $$PWD/cpprofiler\pixeltree\pixel_tree_dialog.cpp \
    $$PWD/cpprofiler\pixeltree\icicle_tree_dialog.cpp \
    $$PWD/cpprofiler\pixeltree\pixel_tree_canvas.cpp \
    $$PWD/cpprofiler\analysis\depth_analysis.cpp \
    $$PWD/cpprofiler\analysis\similar_shapes.cpp \
    $$PWD/cpprofiler\analysis\subtree_comp_win.cpp \
    $$PWD/cpprofiler\analysis\histogram_win.cpp \
    $$PWD/cpprofiler\analysis\subtree_canvas.cpp \
    $$PWD/cpprofiler\analysis\identical_shapes.cpp \
    $$PWD/cpprofiler\analysis\similar_shape_algorithm.cpp \
    $$PWD/cpprofiler\analysis\shape_rect.cpp \
    $$PWD/namemap.cpp

HEADERS  += \
    $$PWD/globalhelper.hh \
    $$PWD/gistmainwindow.h \
    $$PWD/treecanvas.hh \
    $$PWD/visualnode.hh \
    $$PWD/spacenode.hh \
    $$PWD/node.hh \
    $$PWD/node.hpp \
    $$PWD/spacenode.hpp \
    $$PWD/visualnode.hpp \
    $$PWD/heap.hpp \
    $$PWD/nodestats.hh \
    $$PWD/preferences.hh \
    $$PWD/nodewidget.hh \
    $$PWD/drawingcursor.hh \
    $$PWD/drawingcursor.hpp \
    $$PWD/nodecursor_base.hh \
    $$PWD/nodecursor.hh \
    $$PWD/nodecursor.hpp \
    $$PWD/layoutcursor.hh \
    $$PWD/layoutcursor.hpp \
    $$PWD/nodevisitor.hh \
    $$PWD/nodevisitor.hpp \
    $$PWD/zoomToFitIcon.hpp \
    $$PWD/data.hh \
    $$PWD/nodetree.hh \
    $$PWD/highlight_nodes_dialog.hpp \
    $$PWD/cmp_tree_dialog.hh \
    $$PWD/receiverthread.hh \
    $$PWD/treebuilder.hh \
    $$PWD/readingQueue.hh \
    $$PWD/treecomparison.hh \
    $$PWD/nogood_dialog.hh \
    $$PWD/node_info_dialog.hh \
    $$PWD/message.pb.hh \
    $$PWD/profiler-conductor.hh \
    $$PWD/profiler-tcp-server.hh \
    $$PWD/execution.hh \
    $$PWD/tree_utils.hh \
    $$PWD/cpprofiler\pixeltree\pixelImage.hh \
    $$PWD/maybeCaller.hh \
    $$PWD/cpprofiler/analysis/shape_aggregation.hh \
    $$PWD/ml-stats.hh \
    $$PWD/third-party\json.hpp \
    $$PWD/cpprofiler/analysis/backjumps.hh \
    $$PWD/cpprofiler/pixeltree/pixel_data.hh \
    $$PWD/cpprofiler/pixeltree/pixel_tree_dialog.hh \
    $$PWD/cpprofiler/pixeltree/icicle_tree_dialog.hh \
    $$PWD/cpprofiler/pixeltree/pixel_tree_canvas.hh \
    $$PWD/cpprofiler/pixeltree/pixel_item.hh \
    $$PWD/cpprofiler/analysis/depth_analysis.hh \
    $$PWD/cpprofiler/analysis/similar_shapes.hh \
    $$PWD/cpprofiler\analysis\subtree_comp_win.hh \
    $$PWD/cpprofiler\analysis\histogram_win.hh \
    $$PWD/cpprofiler\analysis\subtree_canvas.hh \
    $$PWD/cpprofiler\analysis\identical_shapes.hh \
    $$PWD/cpprofiler\analysis\subtree_analysis.hh \
    $$PWD/cpprofiler\analysis\similar_shape_algorithm.hh \
    $$PWD/cpprofiler\analysis\shape_rect.hh \
    $$PWD/webscript.hh \
    $$PWD/namemap.hh
    

FORMS    +=

own_protobuf {
  SOURCES += \
    $$PWD/protobuf/google/protobuf/stubs/atomicops_internals_x86_gcc.cc         \
    $$PWD/protobuf/google/protobuf/stubs/atomicops_internals_x86_msvc.cc        \
    $$PWD/protobuf/google/protobuf/stubs/common.cc                              \
    $$PWD/protobuf/google/protobuf/stubs/once.cc                                \
    $$PWD/protobuf/google/protobuf/stubs/stringprintf.cc                        \
    $$PWD/protobuf/google/protobuf/extension_set.cc                             \
    $$PWD/protobuf/google/protobuf/generated_message_util.cc                    \
    $$PWD/protobuf/google/protobuf/message_lite.cc                              \
    $$PWD/protobuf/google/protobuf/repeated_field.cc                            \
    $$PWD/protobuf/google/protobuf/wire_format_lite.cc                          \
    $$PWD/protobuf/google/protobuf/io/coded_stream.cc                           \
    $$PWD/protobuf/google/protobuf/io/zero_copy_stream.cc                       \
    $$PWD/protobuf/google/protobuf/io/zero_copy_stream_impl_lite.cc             \
    $$PWD/protobuf/google/protobuf/stubs/strutil.cc                             \
    $$PWD/protobuf/google/protobuf/stubs/substitute.cc                          \
    $$PWD/protobuf/google/protobuf/stubs/structurally_valid.cc                  \
    $$PWD/protobuf/google/protobuf/descriptor.cc                                \
    $$PWD/protobuf/google/protobuf/descriptor.pb.cc                             \
    $$PWD/protobuf/google/protobuf/descriptor_database.cc                       \
    $$PWD/protobuf/google/protobuf/dynamic_message.cc                           \
    $$PWD/protobuf/google/protobuf/extension_set_heavy.cc                       \
    $$PWD/protobuf/google/protobuf/generated_message_reflection.cc              \
    $$PWD/protobuf/google/protobuf/message.cc                                   \
    $$PWD/protobuf/google/protobuf/reflection_ops.cc                            \
    $$PWD/protobuf/google/protobuf/service.cc                                   \
    $$PWD/protobuf/google/protobuf/text_format.cc                               \
    $$PWD/protobuf/google/protobuf/unknown_field_set.cc                         \
    $$PWD/protobuf/google/protobuf/wire_format.cc                               \
    $$PWD/protobuf/google/protobuf/io/gzip_stream.cc                            \
    $$PWD/protobuf/google/protobuf/io/printer.cc                                \
    $$PWD/protobuf/google/protobuf/io/strtod.cc                                 \
    $$PWD/protobuf/google/protobuf/io/tokenizer.cc                              \
    $$PWD/protobuf/google/protobuf/io/zero_copy_stream_impl.cc                  \
    $$PWD/protobuf/google/protobuf/compiler/importer.cc                         \
    $$PWD/protobuf/google/protobuf/compiler/parser.cc
  
  HEADERS += \
    $$PWD/protobuf/google/protobuf/stubs/hash.h                                 \
    $$PWD/protobuf/google/protobuf/stubs/map_util.h                             \
    $$PWD/protobuf/google/protobuf/stubs/shared_ptr.h                           \
    $$PWD/protobuf/google/protobuf/stubs/stringprintf.h                         \
    $$PWD/protobuf/google/protobuf/io/coded_stream_inl.h                        \
    $$PWD/protobuf/google/protobuf/stubs/strutil.h                              \
    $$PWD/protobuf/google/protobuf/stubs/substitute.h
 
  INCLUDEPATH += $$PWD/protobuf 
}

!own_protobuf {
  INCLUDEPATH += /usr/local/include

  LIBS += `pkg-config --cflags --libs protobuf` -lprotobuf
}
