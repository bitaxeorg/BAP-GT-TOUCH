#!/usr/bin/env python3
"""Exercise actual service-start functions for retry and duplicate prevention."""
from pathlib import Path
import subprocess
import tempfile
from test_network_data_contract import function_body
ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/'main/main.c').read_text()
code='''
#include <assert.h>
#include <stddef.h>
#define pdPASS 1
#define ESP_LOGE(...) ((void)0)
static void *price_task_handle, *mempool_task_handle;
static int calls, success;
static void price_task(void *x) {(void)x;}
static void mempool_task(void *x) {(void)x;}
static int xTaskCreate(void (*fn)(void*), const char *name, int stack, void *arg, int priority, void **out) {
 (void)fn;(void)name;(void)stack;(void)arg;(void)priority; calls++; if(success)*out=(void*)1; return success;
}
'''
for name in ('price','mempool'):
    assert name+'_service_start();' in function_body(main,'app_main')
    text=(ROOT/f'main/{name}.c').read_text()
    assert name+'_service_start();' in function_body(text,name+'_screen_create')
    code+='void '+name+'_service_start(void){'+function_body(text,name+'_service_start')+'}\n'
code+='''
int main(void) {
 price_service_start(); mempool_service_start(); assert(calls==2 && !price_task_handle && !mempool_task_handle);
 success=1; price_service_start(); mempool_service_start(); assert(calls==4 && price_task_handle && mempool_task_handle);
 price_service_start(); mempool_service_start(); assert(calls==4);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; c.write_text(code); exe=Path(tmp)/'test'
    subprocess.run(['cc','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: startup owns both services; failed creation retries; screen visits do not duplicate tasks')
