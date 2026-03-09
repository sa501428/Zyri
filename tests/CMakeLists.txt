enable_testing()

function(zyri_add_test target src)
    add_executable(${target} ${src})
    target_link_libraries(${target} PRIVATE ZyriCore ZyriEngine ZyriHistory Qt6::Test)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

# ── Core tests ────────────────────────────────────────────────────────────────
zyri_add_test(TestDocument      core/test_document.cpp)
zyri_add_test(TestLayerStack    core/test_layer_stack.cpp)
zyri_add_test(TestBlendModes    core/test_blend_modes.cpp)

# ── History tests ─────────────────────────────────────────────────────────────
zyri_add_test(TestUndoStack       history/test_undo_stack.cpp)
zyri_add_test(TestDeltaCompressor history/test_delta_compressor.cpp)

# ── Engine tests ──────────────────────────────────────────────────────────────
zyri_add_test(TestPaintEngine  engine/test_paint_engine.cpp)
zyri_add_test(TestFillEngine   engine/test_fill_engine.cpp)
