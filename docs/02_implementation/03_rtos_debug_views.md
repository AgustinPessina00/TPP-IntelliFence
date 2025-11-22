# Solución Rápida: RTOS Views "Busy updating..."

## Problema
ST-Link GDB Server no soporta la opción RTOS automática, causando que RTOS Views se quede en "Busy updating...".

## Solución Inmediata

### Opción 1: Comandos Manuales (Más Rápido)
Cuando el debugger esté pausado, ejecutar desde la **Paleta de Comandos** (Ctrl+Shift+P):

1. `RTOS Views: Refresh`
2. Si no funciona: `RTOS Views: Toggle RTOS Panel`

### Opción 2: Usar Configuración OpenOCD (Más Robusta)
1. Seleccionar la configuración **"CM4_V1 (OpenOCD FreeRTOS)"** en Run and Debug
2. OpenOCD soporta FreeRTOS RTOS automáticamente

### Opción 3: Usar Configuración STM32 Mejorada
1. Seleccionar **"CM4_V1 (STM32 Enhanced)"**
2. Verbose habilitado para mejor debugging
3. Usar comandos manuales de refresh

## Configuraciones Agregadas

### launch.json
- **CM4_V1 (STM32)**: Configuración original
- **CM4_V1 (OpenOCD FreeRTOS)**: Con soporte RTOS automático
- **CM4_V1 (STM32 Enhanced)**: STM32 mejorada con verbose

### settings.json
- Configuraciones de cortex-debug optimizadas
- RTOS Views con auto-refresh habilitado

## Pasos de Troubleshooting

1. **Pausar el debugger** antes de acceder a RTOS Views
2. **Usar la paleta de comandos** para refresh manual
3. **Probar diferentes configuraciones** según disponibilidad de herramientas
4. **Reiniciar VS Code** si persiste el problema

## Nota Importante
El ST-Link GDB Server tiene limitaciones para RTOS Views automáticas. OpenOCD es más robusto para esta funcionalidad, pero requiere configuración adicional del sistema.