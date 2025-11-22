# TPP-IntelliFence - Knowledge Base

## 📚 Documentación del Proyecto

Esta carpeta contiene toda la documentación técnica del proyecto TPP-IntelliFence, un sistema de cerco virtual inteligente para ganado basado en STM32WL55JC.

---

## 📖 Índice de Documentación

### 🏗️ Arquitectura y Diseño

#### Sistema Principal
- **[Arquitectura General](01_architecture/01_system_overview.md)** - Visión general del sistema, componentes principales y flujo de datos
- **[Máquina de Estados (FSM)](01_architecture/02_fsm_architecture.md)** - Diseño de la FSM principal y sub-FSMs
- **[Sistema de Mensajería](01_architecture/03_messaging_system.md)** - Pool-based message system con EmbeddedMessage

#### Modelos de Datos
- **[Clase Cow](01_architecture/04_cow_class.md)** - Modelo de datos de la vaca (posición, aceleración, estado, zona)
- **[Clase Fence](01_architecture/05_fence_class.md)** - Implementación embedded del cerco virtual (sin std::vector)

---

### 🔬 Implementaciones Técnicas

#### RTOS y Debugging
- **[FreeRTOS Best Practices](02_implementation/01_freertos_best_practices.md)** - Guía de buenas prácticas para FreeRTOS
- **[RTOS Printf](02_implementation/02_rtos_printf.md)** - Sistema de logging thread-safe para FreeRTOS
- **[Debug con RTOS Views](02_implementation/03_rtos_debug_views.md)** - Configuración de vistas de depuración RTOS
- **[Watchdog WWDG](02_implementation/04_freertos_wwdg.md)** - Implementación del watchdog window

#### Módulos
- **[GPS Thread-Safe](02_implementation/05_gps_threadsafe.md)** - Implementación thread-safe del módulo GPS

---

### ✅ Testing y Validación

- **[Test Suite Cow & Fence](03_testing/01_cow_fence_tests.md)** - Suite de tests completa para clases Cow y Fence
- **[Test Sistema de Mensajería](03_testing/02_message_system_tests.md)** - Reporte de tests del sistema de mensajería

---

### 📊 Estado del Proyecto

- **[Estado de Desarrollo FSM](04_status/01_fsm_development_status.md)** - Estado actual de implementación de la FSM

---

## 🎯 Quick Start

1. **Nuevo en el proyecto?** → Empieza por [Arquitectura General](01_architecture/01_system_overview.md)
2. **Desarrollando FSM?** → Lee [FSM Architecture](01_architecture/02_fsm_architecture.md)
3. **Debugging RTOS?** → Consulta [RTOS Printf](02_implementation/02_rtos_printf.md) y [Debug Views](02_implementation/03_rtos_debug_views.md)
4. **Agregando tests?** → Revisa [Test Suite](03_testing/01_cow_fence_tests.md)

---

## 📝 Convenciones de Documentación

- **Formato**: Markdown (.md)
- **Estructura**: Secciones organizadas por tema
- **Ejemplos**: Siempre que sea posible, incluir código de ejemplo
- **Diagramas**: Usar ASCII art o referencias a archivos externos

---

## 🔄 Última Actualización

**Fecha**: 22 de Noviembre de 2025  
**Versión**: v1.0  
**Branch**: vscode-refactor
