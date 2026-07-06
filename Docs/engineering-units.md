# Engineering Units

이 문서는 Client, Server, Renderer, Asset 작업에서 사용하는 단위와 좌표 기준을 통일하기 위한 문서입니다. 값의 단위가 코드에서 드러나지 않으면 나중에 이동 속도, 카메라, 조명, 네트워크 동기화가 쉽게 어긋납니다.

## Core Rule

- 내부 계산 단위는 SI 단위를 우선 사용합니다.
- 코드 변수명에는 단위 suffix를 붙입니다.
- 문서, 패킷, 설정 파일에는 단위를 함께 적습니다.
- 예외가 필요한 경우 해당 모듈 문서에 이유를 기록합니다.

## Base Units

| 항목 | 단위 | 코드 suffix | 예시 |
| --- | --- | --- | --- |
| 거리 | meter, `m` | `M` | `positionXM`, `heightM` |
| 시간 | second, `s` | `Sec` | `deltaTimeSec` |
| 짧은 시간 | millisecond, `ms` | `Ms` | `pingMs`, `timeoutMs` |
| 속도 | meter per second, `m/s` | `Mps` | `moveSpeedMps` |
| 가속도 | meter per second squared, `m/s^2` | `Mps2` | `gravityMps2` |
| 각도 | radian, `rad` | `Rad` | `yawRad`, `pitchRad` |
| UI/툴 각도 | degree, `deg` | `Deg` | `fovDeg` |
| 비율 | normalized 0..1 | `01` | `health01`, `alpha01` |
| 개수 | count | `Count` | `playerCount` |
| 인덱스 | index | `Index` | `frameIndex` |

## Coordinate System

MVP 기준 좌표계:

| 축 | 의미 |
| --- | --- |
| `+X` | 오른쪽 |
| `+Y` | 위 |
| `+Z` | 앞 |

기준:

- 좌표계는 Left-handed를 사용합니다.
- 월드 단위는 meter입니다.
- 20x20m 테스트 공간은 `X: -10..10`, `Z: -10..10`을 기본으로 봅니다.
- 캐릭터 발 위치 또는 중심 위치 중 어떤 값을 기준으로 할지 결정해야 합니다. `TODO`
- 지형 높이는 `Y` 값으로 표현합니다.

## Rotation

| 항목 | 기준 |
| --- | --- |
| 내부 계산 | radian |
| 문서/디버그 UI | degree 허용 |
| Yaw | Y축 기준 회전 |
| Pitch | X축 기준 회전 |
| Roll | Z축 기준 회전 |

규칙:

- 삼각함수에 넣는 값은 radian으로 통일합니다.
- 사용자가 보는 설정값이나 디버그 UI는 degree를 허용합니다.
- degree 값을 코드 내부로 가져올 때는 즉시 radian으로 변환합니다.

## Time

| 항목 | 기준 |
| --- | --- |
| Frame delta time | second |
| Network timeout | millisecond |
| Animation time | second |
| Cooldown | second |
| Logging timestamp | TODO |

규칙:

- `Update(float deltaTimeSec)` 형태를 기본으로 합니다.
- 서버 tick 간격도 second 기준으로 계산합니다.
- 네트워크 지연, 타임아웃, RTT는 millisecond 기준으로 기록합니다.

## Movement

| 항목 | 초기값 | 단위 | 비고 |
| --- | --- | --- | --- |
| Player Move Speed | TODO | m/s | MVP 구현 중 조정 |
| Player Radius | TODO | m | 충돌 기준 확정 필요 |
| Player Height | TODO | m | 임시 캡슐/모델 기준 |
| Gravity | `9.8` | m/s^2 | 점프/낙하 구현 시 사용 |

예시:

```cpp
float deltaTimeSec;
float moveSpeedMps;
float playerHeightM;
float yawRad;
```

## Camera

| 항목 | 단위 | 기준 |
| --- | --- | --- |
| FOV | degree in settings, radian internally | 기본값 TODO |
| Near Clip | m | TODO |
| Far Clip | m | TODO |
| Camera Distance | m | TODO |
| Camera Height | m | TODO |

규칙:

- 카메라 설정값은 문서와 디버그 UI에서 degree를 허용합니다.
- Projection Matrix 생성 직전에는 radian으로 변환합니다.
- Near/Far 값은 meter 기준으로 기록합니다.

## Color And Brightness

MVP 단계에서는 물리 기반 조명 단위를 엄격히 적용하지 않고, 렌더링 파이프라인이 안정화될 때까지 선형 색상 기준을 사용합니다.

| 항목 | 기준 | 범위 |
| --- | --- | --- |
| Color internal | Linear RGB | `0.0..1.0` |
| Texture color | sRGB texture 허용 | 로드 시 정책 기록 |
| UI color | sRGB 입력 허용 | 렌더러에서 변환 여부 TODO |
| Alpha | Normalized | `0.0..1.0` |
| Ambient intensity | Unitless | `0.0..1.0` |
| Directional light intensity | Unitless for MVP | TODO |
| Exposure | TODO | Post-MVP |

규칙:

- 셰이더에 전달되는 색상은 가능한 한 Linear RGB 기준으로 맞춥니다.
- 텍스처가 sRGB인지 Linear인지 에셋 문서에 기록합니다.
- 밝기 값은 MVP에서는 `0.0..1.0` 또는 명시된 임시 범위를 사용합니다.
- 실제 조도 단위 `lux`, `nit`, `cd/m^2`는 PBR/HDR 파이프라인을 도입할 때 검토합니다.

## Light Direction

| 항목 | 기준 |
| --- | --- |
| Directional Light Direction | normalized vector |
| Point Light Range | meter |
| Spot Light Angle | radian internally |

규칙:

- 방향 벡터는 정규화해서 전달합니다.
- 빛 방향이 “빛이 향하는 방향”인지 “표면에서 빛을 향하는 방향”인지 셰이더 주석에 명시합니다.
- 라이트 범위는 meter 기준입니다.

## Asset Scale

| 에셋 | 기준 |
| --- | --- |
| Character | 실제 월드 meter 기준 |
| Terrain | 1 unit = 1 m |
| Prop | 1 unit = 1 m |
| Texture | 해상도는 px, 타일링은 meter 기준으로 기록 |

규칙:

- DCC 툴에서 가져온 모델은 import 후 1 unit = 1 m가 되도록 확인합니다.
- 임시 모델도 대략적인 실제 크기를 맞춥니다.
- 에셋 크기 보정이 필요하면 import scale을 `Docs/asset-list.md`에 기록합니다.

## Network Units

패킷에 들어가는 값도 단위를 명확히 합니다.

| 값 | 단위 |
| --- | --- |
| Position | meter |
| Rotation | radian or quaternion, TODO |
| Velocity | m/s |
| Timestamp | ms or tick, TODO |
| Ping / RTT | ms |

규칙:

- 패킷 필드명 또는 문서 설명에 단위를 적습니다.
- 클라이언트와 서버가 같은 좌표계를 사용합니다.
- 압축 또는 정수 양자화를 도입하면 변환 기준을 `Docs/network-protocol.md`에 기록합니다.

## Naming Examples

좋은 예:

```cpp
float deltaTimeSec;
float elapsedTimeMs;
float positionXM;
float moveSpeedMps;
float yawRad;
float fovDeg;
float lightIntensity01;
```

피해야 할 예:

```cpp
float speed;
float time;
float angle;
float brightness;
float pos;
```

## Checklist

새 값을 추가할 때 확인합니다.

- 단위가 문서 또는 변수명에 드러나는가
- Client와 Server가 같은 단위를 쓰는가
- Renderer와 Asset의 스케일이 같은가
- UI/설정값과 내부 계산값의 단위 변환 위치가 명확한가
- 조명/색상 값이 Linear인지 sRGB인지 구분되어 있는가

## Change Log

| 날짜 | 변경 내용 | 이유 |
| --- | --- | --- |
| 2026-07-06 | 단위, 좌표계, 밝기, 에셋 스케일 기준 작성 | 개발 전 단위 혼선 방지 |
