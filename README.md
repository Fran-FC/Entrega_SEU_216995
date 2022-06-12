# Proy_SEU_P6_21699
Uso de FreeRTOS y comandos AT con módulo SP32

Este proyecto consiste en escribir un valor en formato binario (ej."10010001") en el context broker y posteriormente la placa lee ese valor y actualiza el estado de los 8 LEDs.

## Context broker

Para cambiar el valor de los LEDs en el context broker:
``` Bash
$ curl -X POST -H 'Content-Type: application/json' -H 'Accept: application/json' -i 'http://pperez2.disca.upv.es:10000/v1/updateContext' --data \
'{ 
  "contextElements": [{
    "type": "Sensor", "isPattern": "false",
    "id": "SensorSEU_S6_FFC00",    
    "attributes": [{
      "name": "LEDS",
      "type": "binary",
      "value": "01010101"
    }]
  }],
  "updateAction": "APPEND"
}'
```

Y para leer el valor podemos probar con:

```Bash
$ curl -X POST -H 'Content-Type: application/json' -H 'Accept: application/json' \
-i 'http://pperez2.disca.upv.es:10000/v1/queryContext' --data \
'{"
  entities": [{
    "type": "Sensor",
    "isPattern": "false",
    "id": "SensorSEU_S6_FFC00"
  }]
}'
```
