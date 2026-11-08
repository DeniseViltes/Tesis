{
  "graph": {
    "cells": [
      {
        "position": {
          "x": 0,
          "y": 0
        },
        "size": {
          "height": 10,
          "width": 10
        },
        "type": "Statechart",
        "id": "00ffb6d1-d225-4bc0-8b73-7df9987f57b7",
        "attrs": {
          "name": {
            "text": "Controlador bateria Export"
          },
          "specification": {
            "text": "@EventDriven\n@SuperSteps(no)\n\ninterface:\n    in event init\n    in event activar_carga\n    in event activar_descarga\n    in event prueba\n    in event reiniciar\n    in event falla_detectada\n    in event iniciar_automatico\n    in event apagar_automatico\n    \n    var bancos : integer = 0\n    var celdas : integer = 0\n    var rl : integer = 0\n    \n    in event activar_banco\n    in event bypass_banco\n    \n    \n    in event apagar_celda\n    in event encender_celda\n    \n    in event sw_on\n    in event sw_off\n\n    var modo : integer = 0\n    // 0: sin asignar, 1: carga, 2: descarga\n    var celda_modo: integer = 0\n     // 0: fijo, 1: sincro, 2: comp\n     \n     \n    var periodo : integer = 0\n    \n    var llave :boolean\n    "
          }
        },
        "z": 1
      },
      {
        "position": {
          "x": -480,
          "y": -190
        },
        "size": {
          "width": 1118,
          "height": 617
        },
        "type": "Region",
        "attrs": {
          "priority": {
            "text": 1
          },
          "name": {
            "text": "controlador"
          }
        },
        "id": "24cc0e99-a509-40b3-b9e7-463f86a6a931",
        "z": 1463,
        "embeds": [
          "bd05f20d-9250-4a00-a17e-8080c89668fe",
          "156a7dc0-387e-41b6-84c7-cbfe431c21c5",
          "a471ee78-e6d0-427a-bcd1-b4ad6fb82b3d",
          "52794d0c-a25d-4e1f-87b2-8b3922e75183",
          "13c1d113-248d-4483-a292-8b98028982b0",
          "229b855a-7e7d-4600-a518-44f1e5822ede",
          "06c8339d-a126-49fd-b121-785ae33423a5",
          "b7f8c174-f4c0-4109-b1b6-360a5b6fff78",
          "d748d129-0bb2-44eb-b245-b72b40a8b40f",
          "1b53e63e-cce1-4771-8f74-6477042a60a2",
          "4cfd82ff-e926-43b4-ae1d-2774d2bd38ce",
          "3d8a6d1f-19bf-4a52-9e72-9448470931cb",
          "2b81a65c-4d95-4d82-a5dd-b40edcbc55db",
          "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "35db93f4-2981-4ea2-933d-98eff223d9c7"
        ]
      },
      {
        "position": {
          "x": -459,
          "y": -145
        },
        "size": {
          "height": 18,
          "width": 18
        },
        "type": "Entry",
        "entryKind": "Initial",
        "attrs": {},
        "id": "d748d129-0bb2-44eb-b245-b72b40a8b40f",
        "z": 1465,
        "embeds": [
          "4a2f8597-13eb-4800-8fd6-23202281923b"
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "position": {
          "x": -394,
          "y": -134
        },
        "size": {
          "height": 60,
          "width": 145
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "SIN INICIALIZAR",
            "fontSize": 11
          }
        },
        "id": "1b53e63e-cce1-4771-8f74-6477042a60a2",
        "z": 1466,
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Note",
        "attrs": {
          "root": {
            "display": ""
          },
          "body": {
            "filter": {
              "args": {}
            }
          },
          "label": {
            "text": "modo->0: sin asignar, 1: carga, 2: descarga\nrl ->carga del sistema medida en ohms\nbancos->filas de la matriz\nceldas->columnas de la matriz"
          }
        },
        "position": {
          "x": 76,
          "y": -170
        },
        "size": {
          "width": 294.014404296875,
          "height": 67.65854263305664
        },
        "angle": 0,
        "linkable": false,
        "id": "4cfd82ff-e926-43b4-ae1d-2774d2bd38ce",
        "z": 1468,
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "position": {
          "x": -389,
          "y": 39
        },
        "size": {
          "height": 60,
          "width": 153
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "PRUEBA",
            "fontSize": 11
          },
          "specification": {
            "text": "entry / raise bypass_banco\n"
          }
        },
        "id": "3d8a6d1f-19bf-4a52-9e72-9448470931cb",
        "z": 1469,
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931",
        "embeds": []
      },
      {
        "position": {
          "x": 197,
          "y": 59
        },
        "size": {
          "width": 417,
          "height": 340
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "ACTIVO",
            "fontSize": 11
          },
          "specification": {
            "text": "entry / raise bypass_banco\n"
          }
        },
        "id": "2b81a65c-4d95-4d82-a5dd-b40edcbc55db",
        "z": 1470,
        "embeds": [
          "597d54e3-841d-4697-80c2-bfe1ef93c026"
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": -459,
          "y": -130
        },
        "attrs": {
          "label": {
            "refX": "50%",
            "textAnchor": "middle",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "4a2f8597-13eb-4800-8fd6-23202281923b",
        "z": 1480,
        "parent": "d748d129-0bb2-44eb-b245-b72b40a8b40f"
      },
      {
        "position": {
          "x": 200,
          "y": 118
        },
        "size": {
          "width": 411,
          "height": 278
        },
        "type": "Region",
        "attrs": {
          "priority": {
            "text": 1
          }
        },
        "id": "597d54e3-841d-4697-80c2-bfe1ef93c026",
        "z": 1481,
        "embeds": [
          "85a9d9f6-72af-4970-8da6-df324fd4c4a2",
          "c2118f66-ba03-44b6-975c-0b08429653d3",
          "9b38dcd0-b673-423b-8851-1ed73a3a7ece",
          "392fe88f-6f6a-4bfc-b429-7ec57657604b",
          "4bc86f61-c546-4a30-ae78-efc589bfee68",
          "17224f3a-97df-4127-9a76-a649e20613b0",
          "ffc3b2f5-6db6-4c1c-ad22-4a8f35a157b0",
          "cbd98c27-8c6e-4851-8942-6f48b55568c3",
          "eec1445b-b723-4d33-a6cc-c90eeb53c88d"
        ],
        "parent": "2b81a65c-4d95-4d82-a5dd-b40edcbc55db"
      },
      {
        "position": {
          "x": 286,
          "y": 223.5
        },
        "size": {
          "height": 60,
          "width": 60
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "MANUAL",
            "fontSize": 11
          }
        },
        "id": "17224f3a-97df-4127-9a76-a649e20613b0",
        "z": 1482,
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "position": {
          "x": 227,
          "y": 210.5
        },
        "size": {
          "height": 18,
          "width": 18
        },
        "type": "Entry",
        "entryKind": "Initial",
        "attrs": {},
        "id": "ffc3b2f5-6db6-4c1c-ad22-4a8f35a157b0",
        "z": 1483,
        "embeds": [
          "e02a95b7-31c9-40f3-afc6-0112fa83801a"
        ],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "position": {
          "x": 378.014404296875,
          "y": 295.5
        },
        "size": {
          "height": 62,
          "width": 68
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "AUTOMÁTICO",
            "fontSize": 11
          }
        },
        "id": "cbd98c27-8c6e-4851-8942-6f48b55568c3",
        "z": 1484,
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "position": {
          "x": 510,
          "y": 207
        },
        "size": {
          "height": 15,
          "width": 15
        },
        "type": "Exit",
        "attrs": {},
        "id": "eec1445b-b723-4d33-a6cc-c90eeb53c88d",
        "z": 1485,
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026",
        "embeds": [
          "c7bf6246-6af2-4ba6-a810-2b3c08e91491"
        ]
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": 227,
          "y": 225.5
        },
        "attrs": {
          "label": {
            "refX": "50%",
            "textAnchor": "middle",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "e02a95b7-31c9-40f3-afc6-0112fa83801a",
        "z": 1491,
        "parent": "ffc3b2f5-6db6-4c1c-ad22-4a8f35a157b0"
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": 530,
          "y": 207
        },
        "attrs": {
          "label": {
            "text": "falla_activo",
            "refX": 0,
            "textAnchor": "start",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "c7bf6246-6af2-4ba6-a810-2b3c08e91491",
        "z": 1492,
        "parent": "eec1445b-b723-4d33-a6cc-c90eeb53c88d"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "cbd98c27-8c6e-4851-8942-6f48b55568c3"
        },
        "target": {
          "id": "eec1445b-b723-4d33-a6cc-c90eeb53c88d"
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "falla_detectada"
              }
            },
            "position": {
              "distance": 0.31791692546562067,
              "offset": 10.513702392578125,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "85a9d9f6-72af-4970-8da6-df324fd4c4a2",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 477,
            "y": 326.5
          }
        ],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "ffc3b2f5-6db6-4c1c-ad22-4a8f35a157b0"
        },
        "target": {
          "id": "17224f3a-97df-4127-9a76-a649e20613b0",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "11.667%",
              "dy": "46.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {},
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "4bc86f61-c546-4a30-ae78-efc589bfee68",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "cbd98c27-8c6e-4851-8942-6f48b55568c3"
        },
        "target": {
          "id": "17224f3a-97df-4127-9a76-a649e20613b0",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "48.333%",
              "dy": "91.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "apagar_automatico"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "9b38dcd0-b673-423b-8851-1ed73a3a7ece",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 315,
            "y": 337.5
          }
        ],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "17224f3a-97df-4127-9a76-a649e20613b0"
        },
        "target": {
          "id": "cbd98c27-8c6e-4851-8942-6f48b55568c3",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "55.861%",
              "dy": "24.194%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "iniciar_automatico"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "392fe88f-6f6a-4bfc-b429-7ec57657604b",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 394,
            "y": 263.5
          }
        ],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "17224f3a-97df-4127-9a76-a649e20613b0"
        },
        "target": {
          "id": "eec1445b-b723-4d33-a6cc-c90eeb53c88d"
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "falla_detectada"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "c2118f66-ba03-44b6-975c-0b08429653d3",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 372,
            "y": 182
          }
        ],
        "parent": "597d54e3-841d-4697-80c2-bfe1ef93c026"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "d748d129-0bb2-44eb-b245-b72b40a8b40f"
        },
        "target": {
          "id": "1b53e63e-cce1-4771-8f74-6477042a60a2",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "19.31%",
              "dy": "60%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {},
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "b7f8c174-f4c0-4109-b1b6-360a5b6fff78",
        "z": 1493,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "position": {
          "x": 690,
          "y": -200
        },
        "size": {
          "width": 603,
          "height": 210
        },
        "type": "Region",
        "attrs": {
          "priority": {
            "text": 2
          },
          "name": {
            "text": "Banco"
          }
        },
        "id": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789",
        "z": 1566,
        "embeds": [
          "b04ed815-f46a-4719-9ba8-3804cf3d0def",
          "41a32706-1235-4248-b7fe-f51729eb51b0",
          "1c671289-571d-4754-b792-96829c39c54a",
          "4a93f8ca-df38-41af-aefd-d2bc2f903a8f",
          "51ae6d22-7da7-4367-9c4d-90824eeb966d",
          "9e2ee851-cab9-4d42-8ecc-d53a381257fc",
          "6fe7f2f6-4534-4788-9bc9-b3c307415c96",
          "95b3eb04-ccb4-4d4f-80da-275f8e060693",
          "d3fafc33-c6c7-4fcb-b79d-a3565750f7bb"
        ]
      },
      {
        "position": {
          "x": 879,
          "y": -95
        },
        "size": {
          "height": 60,
          "width": 145
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "BYPASS ON",
            "fontSize": 11
          },
          "documentation": {
            "text": "\n"
          }
        },
        "id": "51ae6d22-7da7-4367-9c4d-90824eeb966d",
        "z": 1567,
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "type": "Note",
        "attrs": {
          "root": {
            "display": ""
          },
          "body": {
            "filter": {
              "args": {}
            }
          },
          "label": {
            "text": "Banco ON: Todas las \nceldas encendidas\nBanco OFF: Bypass"
          }
        },
        "position": {
          "x": 764,
          "y": -180
        },
        "size": {
          "width": 141.9495849609375,
          "height": 60
        },
        "angle": 0,
        "linkable": false,
        "id": "9e2ee851-cab9-4d42-8ecc-d53a381257fc",
        "z": 1568,
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "position": {
          "x": 1150,
          "y": -130
        },
        "size": {
          "height": 66,
          "width": 130
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "BYPASS OFF",
            "fontSize": 11
          }
        },
        "id": "6fe7f2f6-4534-4788-9bc9-b3c307415c96",
        "z": 1569,
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "position": {
          "x": 720,
          "y": -160
        },
        "size": {
          "height": 18,
          "width": 18
        },
        "type": "Entry",
        "entryKind": "Initial",
        "attrs": {},
        "id": "95b3eb04-ccb4-4d4f-80da-275f8e060693",
        "z": 1572,
        "embeds": [
          "7d0ff814-78a1-41c0-9ec2-635efc0f19cb"
        ],
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "position": {
          "x": 750,
          "y": -90
        },
        "size": {
          "height": 60,
          "width": 90
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "DESHABILITADO",
            "fontSize": 11
          }
        },
        "id": "d3fafc33-c6c7-4fcb-b79d-a3565750f7bb",
        "z": 1573,
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": 720,
          "y": -145
        },
        "attrs": {
          "label": {
            "refX": "50%",
            "textAnchor": "middle",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "7d0ff814-78a1-41c0-9ec2-635efc0f19cb",
        "z": 1576,
        "parent": "95b3eb04-ccb4-4d4f-80da-275f8e060693"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "95b3eb04-ccb4-4d4f-80da-275f8e060693"
        },
        "target": {
          "id": "d3fafc33-c6c7-4fcb-b79d-a3565750f7bb",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "33.333%",
              "dy": "66.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {},
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "b04ed815-f46a-4719-9ba8-3804cf3d0def",
        "z": 1577,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 729,
            "y": -90
          }
        ],
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "d3fafc33-c6c7-4fcb-b79d-a3565750f7bb"
        },
        "target": {
          "id": "51ae6d22-7da7-4367-9c4d-90824eeb966d",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "21.379%",
              "dy": "58.333%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "[llave == true]"
              }
            },
            "position": {
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "41a32706-1235-4248-b7fe-f51729eb51b0",
        "z": 1577,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 860,
            "y": 0
          }
        ],
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "6fe7f2f6-4534-4788-9bc9-b3c307415c96",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "71.538%",
              "dy": "75.758%",
              "rotate": true
            }
          },
          "priority": true
        },
        "target": {
          "id": "51ae6d22-7da7-4367-9c4d-90824eeb966d",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "80%",
              "dy": "58.333%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "bypass_banco/\nraise apagar_celda "
              }
            },
            "position": {
              "distance": 0.5531505904948784,
              "offset": -27.38289422268585,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "4a93f8ca-df38-41af-aefd-d2bc2f903a8f",
        "z": 1577,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1090,
            "y": -80
          }
        ],
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "51ae6d22-7da7-4367-9c4d-90824eeb966d"
        },
        "target": {
          "id": "6fe7f2f6-4534-4788-9bc9-b3c307415c96",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "15%",
              "dy": "21.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "activar_banco / \nraise encender_celda "
              }
            },
            "position": {
              "distance": 0.4602120465752061,
              "offset": -16.873123168945312,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "1c671289-571d-4754-b792-96829c39c54a",
        "z": 1577,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1032,
            "y": -115.69999999999999
          }
        ],
        "parent": "093d6d63-8cbb-4fa1-a541-d5a9ffacf789"
      },
      {
        "position": {
          "x": 710,
          "y": 70
        },
        "size": {
          "width": 1000,
          "height": 441
        },
        "type": "Region",
        "attrs": {
          "priority": {
            "text": 3
          },
          "name": {
            "text": "celda"
          }
        },
        "id": "1276dc20-336d-431f-a960-d2712e3bd3b0",
        "z": 1603,
        "embeds": [
          "0113b8f6-4baf-4ff5-b1a0-d64d79008e6e",
          "6cb8f0af-4f09-4899-b940-bcd7b4ea0539",
          "a4762879-6e15-4a75-abc3-250410a814aa",
          "2534818e-aa70-4754-bb2f-84aaadb11536",
          "285adf70-a32b-492b-bc8c-bca6e2ce0b70",
          "ee90457f-b119-46e3-850a-2ab887e2cd9f",
          "f4acde28-c6cb-4839-a9a9-1d0661b87bff",
          "06434b05-de08-4b9c-b7d5-0eaba60734d3",
          "59f89280-abc9-48bc-b9d2-cd85eddf564f"
        ]
      },
      {
        "type": "Note",
        "attrs": {
          "root": {
            "display": ""
          },
          "body": {
            "filter": {
              "args": {}
            }
          },
          "label": {
            "text": "Comportamiento del circuito\n para una única celda"
          }
        },
        "position": {
          "x": 830,
          "y": 391
        },
        "size": {
          "width": 190,
          "height": 60
        },
        "angle": 0,
        "linkable": false,
        "id": "285adf70-a32b-492b-bc8c-bca6e2ce0b70",
        "z": 1604,
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": 1100,
          "y": 171
        },
        "size": {
          "height": 295,
          "width": 596
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "ON",
            "fontSize": 11
          }
        },
        "id": "ee90457f-b119-46e3-850a-2ab887e2cd9f",
        "z": 1605,
        "embeds": [
          "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
        ],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": 800,
          "y": 211
        },
        "size": {
          "height": 60,
          "width": 160
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "OFF",
            "fontSize": 11
          },
          "specification": {
            "text": "entry / raise bypass_banco"
          }
        },
        "id": "f4acde28-c6cb-4839-a9a9-1d0661b87bff",
        "z": 1606,
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": 820,
          "y": 91
        },
        "size": {
          "height": 60,
          "width": 120
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "DESHABILITADA",
            "fontSize": 11
          }
        },
        "id": "06434b05-de08-4b9c-b7d5-0eaba60734d3",
        "z": 1609,
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": 730,
          "y": 181
        },
        "size": {
          "height": 18,
          "width": 18
        },
        "type": "Entry",
        "entryKind": "Initial",
        "attrs": {},
        "id": "59f89280-abc9-48bc-b9d2-cd85eddf564f",
        "z": 1610,
        "embeds": [
          "279ee433-d8cc-4e0b-bceb-de9cdb0fe66c"
        ],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": 1103,
          "y": 216
        },
        "size": {
          "width": 590,
          "height": 247
        },
        "type": "Region",
        "attrs": {
          "priority": {
            "text": 1
          }
        },
        "id": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b",
        "z": 1612,
        "embeds": [
          "07c5db08-bc3a-4e69-9b27-43dae29c973b",
          "92a98364-ca47-4c0c-a7ab-d9a854f79931",
          "db5e7bea-79a0-4b11-bc57-bcc042833498",
          "6148bce1-660b-43f3-ad7c-ae541998d850",
          "247380c4-ef30-4be7-b499-2b974019592c",
          "3554b79e-dc75-4d4b-8669-7f88eb41fde5",
          "6c43c52a-7223-41ea-a702-c5eb315a0f0b",
          "e8fdc949-3388-4337-8b43-d2002685d0aa",
          "bd12d7e6-4418-4534-bf32-2548df6a0405",
          "bc348896-9807-4621-8dab-b7ccdb24badf",
          "a7333a7b-5e3f-4d1f-a0ae-9894b57be032"
        ],
        "parent": "ee90457f-b119-46e3-850a-2ab887e2cd9f"
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": 730,
          "y": 196
        },
        "attrs": {
          "label": {
            "refX": "50%",
            "textAnchor": "middle",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "279ee433-d8cc-4e0b-bceb-de9cdb0fe66c",
        "z": 1613,
        "parent": "59f89280-abc9-48bc-b9d2-cd85eddf564f"
      },
      {
        "position": {
          "x": 1206,
          "y": 335
        },
        "size": {
          "height": 60,
          "width": 100
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "FIJO",
            "fontSize": 11
          },
          "specification": {
            "text": "entry / modo = 0"
          }
        },
        "id": "e8fdc949-3388-4337-8b43-d2002685d0aa",
        "z": 1614,
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "position": {
          "x": 1126,
          "y": 365
        },
        "size": {
          "height": 15,
          "width": 15
        },
        "type": "Entry",
        "entryKind": "Initial",
        "attrs": {},
        "id": "bd12d7e6-4418-4534-bf32-2548df6a0405",
        "z": 1615,
        "embeds": [
          "2f149126-d956-4cc9-93d3-37305c7e0f99"
        ],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "position": {
          "x": 1506,
          "y": 365
        },
        "size": {
          "height": 60,
          "width": 120
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "COMPLEMENTARIO",
            "fontSize": 11
          }
        },
        "id": "bc348896-9807-4621-8dab-b7ccdb24badf",
        "z": 1616,
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "position": {
          "x": 1520,
          "y": 236
        },
        "size": {
          "height": 60,
          "width": 110
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "SINCRONIZADO",
            "fontSize": 11
          }
        },
        "id": "a7333a7b-5e3f-4d1f-a0ae-9894b57be032",
        "z": 1617,
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "NodeLabel",
        "label": true,
        "size": {
          "width": 15,
          "height": 15
        },
        "position": {
          "x": 1126,
          "y": 380
        },
        "attrs": {
          "label": {
            "refX": "50%",
            "textAnchor": "middle",
            "refY": "50%",
            "textVerticalAnchor": "middle"
          }
        },
        "id": "2f149126-d956-4cc9-93d3-37305c7e0f99",
        "z": 1625,
        "parent": "bd12d7e6-4418-4534-bf32-2548df6a0405"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "59f89280-abc9-48bc-b9d2-cd85eddf564f"
        },
        "target": {
          "id": "06434b05-de08-4b9c-b7d5-0eaba60734d3",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "8.333%",
              "dy": "50%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {},
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "6cb8f0af-4f09-4899-b940-bcd7b4ea0539",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 760,
            "y": 121
          }
        ],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "a7333a7b-5e3f-4d1f-a0ae-9894b57be032"
        },
        "target": {
          "id": "bc348896-9807-4621-8dab-b7ccdb24badf",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "50%",
              "dy": "0%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_on [modo == 2]"
              }
            },
            "position": {
              "offset": -55,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "db5e7bea-79a0-4b11-bc57-bcc042833498",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "bc348896-9807-4621-8dab-b7ccdb24badf"
        },
        "target": {
          "id": "a7333a7b-5e3f-4d1f-a0ae-9894b57be032",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "37.5%",
              "dy": "100%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_on [modo == 1]"
              }
            },
            "position": {
              "offset": -55,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "07c5db08-bc3a-4e69-9b27-43dae29c973b",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "bd12d7e6-4418-4534-bf32-2548df6a0405"
        },
        "target": {
          "id": "e8fdc949-3388-4337-8b43-d2002685d0aa",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "43.333%",
              "dy": "55.833%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {},
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "6c43c52a-7223-41ea-a702-c5eb315a0f0b",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "bc348896-9807-4621-8dab-b7ccdb24badf"
        },
        "target": {
          "id": "e8fdc949-3388-4337-8b43-d2002685d0aa",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "70%",
              "dy": "100%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_off"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "3554b79e-dc75-4d4b-8669-7f88eb41fde5",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1436,
            "y": 425
          }
        ],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "a7333a7b-5e3f-4d1f-a0ae-9894b57be032"
        },
        "target": {
          "id": "e8fdc949-3388-4337-8b43-d2002685d0aa",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "30%",
              "dy": "16.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_off"
              }
            },
            "position": {
              "distance": 0.35558762050887294,
              "offset": 5.000000000000028,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "6148bce1-660b-43f3-ad7c-ae541998d850",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1410,
            "y": 241
          }
        ],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "e8fdc949-3388-4337-8b43-d2002685d0aa"
        },
        "target": {
          "id": "bc348896-9807-4621-8dab-b7ccdb24badf",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "3.333%",
              "dy": "68.333%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_on [celda_modo == 2 && periodo !=0]"
              }
            },
            "position": {
              "distance": 0.6051815667164605,
              "offset": -50.843353271484375,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "247380c4-ef30-4be7-b499-2b974019592c",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1350,
            "y": 365
          }
        ],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "e8fdc949-3388-4337-8b43-d2002685d0aa"
        },
        "target": {
          "id": "a7333a7b-5e3f-4d1f-a0ae-9894b57be032",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "25%",
              "dy": "50%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "sw_on [celda_modo == 1 && periodo !=0]"
              }
            },
            "position": {
              "distance": 0.6022674768617026,
              "offset": 15,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "92a98364-ca47-4c0c-a7ab-d9a854f79931",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 1280,
            "y": 291
          }
        ],
        "parent": "d5bf3446-dfc2-462e-8fae-efb9b4f02e9b"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "f4acde28-c6cb-4839-a9a9-1d0661b87bff"
        },
        "target": {
          "x": 2322,
          "y": 348,
          "id": "ee90457f-b119-46e3-850a-2ab887e2cd9f",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "0.619%",
              "dy": "49.197%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "encender_celda /\nraise activar_banco "
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "2534818e-aa70-4754-bb2f-84aaadb11536",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 820,
            "y": 316.13
          }
        ],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "ee90457f-b119-46e3-850a-2ab887e2cd9f"
        },
        "target": {
          "id": "f4acde28-c6cb-4839-a9a9-1d0661b87bff",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "97.692%",
              "dy": "48.333%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "apagar_celda /\nraise activar_banco"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "a4762879-6e15-4a75-abc3-250410a814aa",
        "z": 1626,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "06434b05-de08-4b9c-b7d5-0eaba60734d3"
        },
        "target": {
          "id": "f4acde28-c6cb-4839-a9a9-1d0661b87bff",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "50%",
              "dy": "15%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "[llave == true]"
              }
            },
            "position": {
              "offset": -55,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "0113b8f6-4baf-4ff5-b1a0-d64d79008e6e",
        "z": 1627,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "1276dc20-336d-431f-a960-d2712e3bd3b0"
      },
      {
        "position": {
          "x": -160,
          "y": -50
        },
        "size": {
          "height": 60,
          "width": 100
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "REPOSO",
            "fontSize": 11
          }
        },
        "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
        "z": 1628,
        "embeds": [
          "1804327a-d760-46b9-8b77-fa09a8830276"
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "3d8a6d1f-19bf-4a52-9e72-9448470931cb"
        },
        "target": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "13.333%",
              "dy": "100%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "reiniciar /\nmodo = 0; rl = 0"
              }
            },
            "position": {
              "distance": 0.37095370560332236,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "a471ee78-e6d0-427a-bcd1-b4ad6fb82b3d",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": -177,
            "y": 49
          }
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "2b81a65c-4d95-4d82-a5dd-b40edcbc55db",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "23.333%",
              "dy": "0%",
              "rotate": true
            }
          },
          "priority": true
        },
        "target": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "53.333%",
              "dy": "80%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "reiniciar / modo = 0; rl = 0"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "52794d0c-a25d-4e1f-87b2-8b3922e75183",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": 118,
            "y": -2
          }
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "1b53e63e-cce1-4771-8f74-6477042a60a2"
        },
        "target": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "46.667%",
              "dy": "38.333%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "init [bancos > 0 && bancos <= 4 && \nceldas > 0 && celdas <=7] \n/ modo = 0; rl = 0; llave = false"
              }
            },
            "position": {
              "distance": 0.6199549452750419,
              "offset": -20.31677005030775,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "13c1d113-248d-4483-a292-8b98028982b0",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [
          {
            "x": -59,
            "y": -104
          }
        ],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc"
        },
        "target": {
          "id": "2b81a65c-4d95-4d82-a5dd-b40edcbc55db",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "55%",
              "dy": "50%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "activar_descarga [rl > 0 && rl <= 1000] / modo = 2; llave = true"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "229b855a-7e7d-4600-a518-44f1e5822ede",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc"
        },
        "target": {
          "id": "3d8a6d1f-19bf-4a52-9e72-9448470931cb",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "51.667%",
              "dy": "91.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "prueba / llave = true"
              }
            },
            "position": {
              "distance": 0.39292440147435453,
              "offset": 16,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "06c8339d-a126-49fd-b121-785ae33423a5",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc"
        },
        "target": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "85%",
              "dy": "86.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "init [bancos > 0 \n&& bancos <= 4 && \nceldas > 0 && celdas <=7] \n/ modo = 0; rl = 0"
              }
            },
            "position": {
              "distance": 0.7564518863756017,
              "offset": 77.11582066177125,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "3"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "1804327a-d760-46b9-8b77-fa09a8830276",
        "z": 1629,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc"
      },
      {
        "position": {
          "x": -180,
          "y": 150
        },
        "size": {
          "width": 140,
          "height": 60
        },
        "type": "State",
        "attrs": {
          "name": {
            "text": "FALLA",
            "fontSize": 11
          },
          "specification": {
            "text": "entry / llave = false"
          }
        },
        "id": "35db93f4-2981-4ea2-933d-98eff223d9c7",
        "z": 1630,
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "2b81a65c-4d95-4d82-a5dd-b40edcbc55db"
        },
        "target": {
          "id": "35db93f4-2981-4ea2-933d-98eff223d9c7",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "70%",
              "dy": "71.667%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "# falla_activo >"
              }
            },
            "position": {}
          },
          {
            "attrs": {
              "label": {
                "text": "2"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "bd05f20d-9250-4a00-a17e-8080c89668fe",
        "z": 1631,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      },
      {
        "type": "Transition",
        "attrs": {},
        "source": {
          "id": "35db93f4-2981-4ea2-933d-98eff223d9c7"
        },
        "target": {
          "id": "b5a9a5f0-5de4-4e88-a954-648ddcbfb6bc",
          "anchor": {
            "name": "topLeft",
            "args": {
              "dx": "36.667%",
              "dy": "75%",
              "rotate": true
            }
          },
          "priority": true
        },
        "connector": {
          "name": "rounded"
        },
        "labels": [
          {
            "attrs": {
              "text": {
                "text": "init [bancos > 0 && bancos <= 4 && \nceldas > 0 && celdas <=7] \n/ modo = 0; rl = 0"
              }
            },
            "position": {
              "distance": 0.14285714285714285,
              "offset": 96.00000007629396,
              "angle": 0
            }
          },
          {
            "attrs": {
              "label": {
                "text": "1"
              }
            }
          },
          {
            "attrs": {}
          },
          {
            "attrs": {}
          }
        ],
        "id": "156a7dc0-387e-41b6-84c7-cbfe431c21c5",
        "z": 1631,
        "router": {
          "name": "orthogonal"
        },
        "vertices": [],
        "parent": "24cc0e99-a509-40b3-b9e7-463f86a6a931"
      }
    ]
  },
  "genModel": {
    "generator": {
      "type": "create::c",
      "features": {
        "Outlet": {
          "targetProject": "",
          "targetFolder": "",
          "libraryTargetFolder": "",
          "skipLibraryFiles": "",
          "apiTargetFolder": ""
        },
        "LicenseHeader": {
          "licenseText": ""
        },
        "FunctionInlining": {
          "inlineReactions": false,
          "inlineEntryActions": false,
          "inlineExitActions": false,
          "inlineEnterSequences": false,
          "inlineExitSequences": false,
          "inlineChoices": false,
          "inlineEnterRegion": false,
          "inlineExitRegion": false,
          "inlineEntries": false
        },
        "OutEventAPI": {
          "observables": false,
          "getters": false
        },
        "IdentifierSettings": {
          "moduleName": "ControladorBateria",
          "statemachinePrefix": "controladorBateria",
          "separator": "_",
          "headerFilenameExtension": "h",
          "sourceFilenameExtension": "c"
        },
        "Tracing": {
          "enterState": false,
          "exitState": false,
          "generic": false
        },
        "Includes": {
          "useRelativePaths": false,
          "generateAllSpecifiedIncludes": false
        },
        "GeneratorOptions": {
          "userAllocatedQueue": false,
          "metaSource": false
        },
        "GeneralFeatures": {
          "timerService": false,
          "timerServiceTimeType": ""
        },
        "Debug": {
          "dumpSexec": false
        }
      }
    }
  }
}