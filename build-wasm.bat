@echo off
where emcc >nul 2>nul
if errorlevel 1 (
    echo Error: emcc was not found. Activate or install the Emscripten SDK first.
    exit /b 1
)

emcc vmm.c vmm_web.c -std=c99 -O2 --no-entry -s MODULARIZE=1 -s EXPORT_NAME=createVMMModule -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['HEAPU8'] -s EXPORTED_FUNCTIONS=['_malloc','_free','_vmm_web_init','_vmm_web_destroy','_vmm_web_step','_vmm_web_done','_vmm_web_position','_vmm_web_hits','_vmm_web_faults','_vmm_web_hit_ratio','_vmm_web_event_page','_vmm_web_event_hit','_vmm_web_event_frame','_vmm_web_event_has_evicted','_vmm_web_event_evicted','_vmm_web_frame_count','_vmm_web_frame_occupied','_vmm_web_frame_page'] -o vmm.js
if errorlevel 1 exit /b 1
echo Built vmm.js and vmm.wasm for the browser UI.
