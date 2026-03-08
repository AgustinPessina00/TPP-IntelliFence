# UML Diagrams — TPP-IntelliFence FSMs

Diagramas PlantUML de las máquinas de estado (FSM) del firmware CM4.

## Archivos

### FSM — Máquinas de estado

| Archivo | FSM | Descripción |
|---|---|---|
| [`main_fsm.puml`](main_fsm.puml) | `MainFSM_t` | FSM principal de nivel superior |
| [`startup_routine_fsm.puml`](startup_routine_fsm.puml) | `StartupRoutineState_t` | Rutina de inicio (join LoRa, GPS, cerca) |
| [`normal_operation_fsm.puml`](normal_operation_fsm.puml) | `NormalOpFSM_t` + sub-FSMs | Operación normal con sub-FSMs compuestas |
| [`fence_transition_fsm.puml`](fence_transition_fsm.puml) | `FenceTransitionState_t` | Transición al recibir una nueva cerca |

### Clases

| Archivo | Clase(s) | Descripción |
|---|---|---|
| [`gps_sam_m10q_class.puml`](gps_sam_m10q_class.puml) | `SamM10q` | Driver GPS u-blox SAM-M10Q (UBX/I2C/UART) |
| [`ina226_class.puml`](ina226_class.puml) | `Ina226` | Monitor de corriente/potencia INA226 (×3 instancias) |
| [`lsm6dso_class.puml`](lsm6dso_class.puml) | `Lsm6dso` | IMU 6DOF ST LSM6DSO (acelerómetro + giróscopo) |
| [`cow_fence_class.puml`](cow_fence_class.puml) | `Cow`, `Fence` | Clases de dominio: vaca y cerco virtual |
| [`communication_buses_class.puml`](communication_buses_class.puml) | `II2C`, `I2CBus`, `I2CManager`, `IUART`, `UARTBus`, `UARTManager` | Infraestructura de comunicación I2C y UART (thread-safe) |

## Jerarquía de FSMs

```
MainFSM_t
├── STARTUP_ROUTINE     → StartupRoutineState_t
├── NORMAL_OPERATION    → NormalOpFSM_t
│   ├── INITIALIZE      → InitializeState_t
│   ├── GREEN_ZONE      → GreenZoneState_t
│   └── STIMULUS_ZONE   → StimulusZone_t
└── FENCE_TRANSITION    → FenceTransitionState_t
```

## Cómo visualizar

### Opción 1 — Extensión VS Code (recomendada)
Instalar [PlantUML](https://marketplace.visualstudio.com/items?itemName=jebbs.plantuml) y hacer **Alt+D** con el archivo `.puml` abierto.

Requiere Java y el servidor remoto o PlantUML local:
- Servidor remoto (sin instalar nada extra): agregar en `settings.json`:
  ```json
  "plantuml.render": "PlantUMLServer",
  "plantuml.server": "https://www.plantuml.com/plantuml"
  ```

### Opción 2 — PlantUML online
Pegar el contenido del archivo en [https://www.plantuml.com/plantuml/uml/](https://www.plantuml.com/plantuml/uml/)

### Opción 3 — CLI local
```sh
java -jar plantuml.jar docs/06_uml/*.puml
```

## Archivos fuente (C++)

| Diagrama | Implementación |
|---|---|
| Main FSM | `CM4/Core/Src/threads/fsmTask.cpp` |
| Startup Routine | `CM4/Core/Src/threads/startupRoutineFsm.cpp` |
| Normal Operation | `CM4/Core/Src/threads/normalOperationFsm.cpp` |
| Fence Transition | `CM4/Core/Src/threads/fenceTransitionFsm.cpp` |
| Tipos/enums | `CM4/Core/Inc/threads/fsmTask.h` |
