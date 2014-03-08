@echo off
echo **** vg_shader_fd_4.c.qasm
echo vg_shader_fd_4.c
qasm -w -minline_c -tb0 vg_shader_fd_4.c.qasm >vg_shader_fd_4.c
echo.
echo **** vg_shader_md_4.c.qasm
echo vg_shader_md_4.c
qasm -w -minline_c -tb0 vg_shader_md_4.c.qasm >vg_shader_md_4.c
echo.
echo **** vg_tess_init_4.qasm
echo vg_tess_init_shader_4.c
qasm -w -mml_c:middleware/khronos/vg/2708/vg_tess_init_shader_4,VG_TESS_INIT_SHADER_4,vg_tess_init_shader,annots -tb0 vg_tess_init_4.qasm >vg_tess_init_shader_4.c
echo vg_tess_init_shader_4.h
qasm -w -mml_h:middleware/khronos/vg/2708/vg_tess_init_shader_4,VG_TESS_INIT_SHADER_4,vg_tess_init_shader,annots -tb0 vg_tess_init_4.qasm >vg_tess_init_shader_4.h
echo.
echo **** vg_tess_term_4.qasm
echo vg_tess_term_shader_4.c
qasm -w -mml_c:middleware/khronos/vg/2708/vg_tess_term_shader_4,VG_TESS_TERM_SHADER_4,vg_tess_term_shader,annots -tb0 vg_tess_term_4.qasm >vg_tess_term_shader_4.c
echo vg_tess_term_shader_4.h
qasm -w -mml_h:middleware/khronos/vg/2708/vg_tess_term_shader_4,VG_TESS_TERM_SHADER_4,vg_tess_term_shader,annots -tb0 vg_tess_term_4.qasm >vg_tess_term_shader_4.h
echo.
echo **** vg_tess_fill_4.qasm
echo vg_tess_fill_shader_4.c
qasm -w -mml_c:middleware/khronos/vg/2708/vg_tess_fill_shader_4,VG_TESS_FILL_SHADER_4,vg_tess_fill_shader,annots -tb0 vg_tess_fill_4.qasm >vg_tess_fill_shader_4.c
echo vg_tess_fill_shader_4.h
qasm -w -mml_h:middleware/khronos/vg/2708/vg_tess_fill_shader_4,VG_TESS_FILL_SHADER_4,vg_tess_fill_shader,annots -tb0 vg_tess_fill_4.qasm >vg_tess_fill_shader_4.h
echo vg_tess_fill_aliases_4.h
qasm -w -maliases -tb0 vg_tess_fill_4.qasm >vg_tess_fill_aliases_4.h
echo.
echo **** vg_tess_stroke_4.qasm
echo vg_tess_stroke_shader_4.c
qasm -w -mml_c:middleware/khronos/vg/2708/vg_tess_stroke_shader_4,VG_TESS_STROKE_SHADER_4,vg_tess_stroke_shader,annots -tb0 vg_tess_stroke_4.qasm >vg_tess_stroke_shader_4.c
echo vg_tess_stroke_shader_4.h
qasm -w -mml_h:middleware/khronos/vg/2708/vg_tess_stroke_shader_4,VG_TESS_STROKE_SHADER_4,vg_tess_stroke_shader,annots -tb0 vg_tess_stroke_4.qasm >vg_tess_stroke_shader_4.h
echo vg_tess_stroke_aliases_4.h
qasm -w -maliases -tb0 vg_tess_stroke_4.qasm >vg_tess_stroke_aliases_4.h
