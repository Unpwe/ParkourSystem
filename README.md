![Hop](https://github.com/user-attachments/assets/c6eb5005-3284-474b-92b8-a406b4f31a2b)

# [Parkour System C++]
언리얼엔진5에서 활용가능한 Parkour 플러그인


## 목적
**❶**: 플러그인을 적용하고 캐릭터의 기본 정보들만을 셋업해주기만 하면 사용가능 한 파쿠르 플러그인을 제작. <br />
블루프린트를 잘 알지 못해도 블루프린트에서 핵심 함수만 호출하면 가능하도록 설계.

**❷**: 상세하게 수치를 커스텀할 수 있게하여 사용자에게 설정 자유도를 허락.

**❸**: 아이템이나 외부적인 요인으로 인해 파쿠르가 가능해지는 범위가 늘어나는 등 수치를 이용하여 파쿠르 범위가 늘어날수 있는 유연성 확보.

**❹**: 파쿠르 키는 커맨드 교체가 가능 하도록 설계.

**❺**: 파쿠르 키는 하나의 키만을 사용하기 때문에 방향키와 캐릭터의 파쿠르 상태에 따라 **상태 패턴** 과 **전략 패턴**으로 **캡슐화된 알고리즘이 상호 교체되도록 설계.

**❻**:  Motion Warping을 이용하여 애니메이션의 기본 루트 위치와 관계 없이 사용자가 커스텀하게 수치를 입력하여 조정 가능하도록 설계.


<br>

## 1. Initialize

### ❖ Set intialize Reference
---
![Initialize](https://github.com/user-attachments/assets/593c7d89-6001-47bf-821b-61ed6833cce1)

파쿠르 상태를 계산하기 위해 캐릭터의 주요 정보들을 **[ParkourComponent]** 에 기입하는 작업.

파쿠르를 사용 할 캐릭터의 블루프린트에서 **[Set Initialize Reference]** 함수에 파쿠르를 실행할 때 사용해야하는 캐릭터의 정보들을 기입해 주어야함.

> **Trace Type query**는 언리얼 에디터 **[편집] -> [프로젝트 세팅] -> [콜리전]** 항목에서 **[새 트레이스 채널]** 버튼을 눌러서 생성한 Trace 채널을 이용한다.

<br>

### ❖ Movement
---
![Movement](https://github.com/user-attachments/assets/f0369689-5593-48b7-8647-38891a66833a)

캐릭터의 입력 액션에 파쿠르 Movement를 연동시키는 작업.

캐릭터의 [InputAction]을 사용하며, **[Triggered] -> [Movement Input Callable]** / **[Completed] -> [Reset Movement Callable]** 을 연결 해주어야 한다.

> FVector2D 타입의 **[Action Value]** 의 핀을 분할하여 X는 **[Movement Input Callable]** Front = true, Y는 **[Movement Input Callable]** Front = false를 사용한다.

<br>

### ❖ Parkour Callable
---
![Parkour Callable](https://github.com/user-attachments/assets/d4ef2115-85ec-49ce-8585-2b8f49370355)

파쿠르를 실행하는 실질적인 함수. <br />

**[Play Parkour Callable]** 함수를 실행하면 내부에서 캐릭터 앞의 벽의 높이, 깊이, 너비를 계산하여 상황에 맞는 파쿠르를 진행한다.

> **[Play Parkour Callable]** : 파쿠르 실행 함수 실행 <br />
> **[Parkour Drop Callable]** : **Climb에 해당하는 파쿠르 실행 중에** 취소하고 떨어지는 함수 실행 <br />
> **[Parkour Cancel Callable]** : 파쿠르를 강제로 Reset 시키는 함수 실행 <br />

<br>

## 2. 파쿠르 상태
![ParkourState](https://github.com/user-attachments/assets/21112b7a-5158-4172-9846-d2b12a1e750a)

* **[Climb]** : Climb 상태 중 <br />
* **[Mantle]** : Mantle하는 중인 상태 <br />
* **[NotBusy]** : 아무행동도 하지 않는 상태 <br />
* **[ReachLedge]** : Climb/Hop을 시도하는 상태 <br />
* **[Vault]** : Vault하는 중인 상태 <br />

<br>

## 3. Parkour Data Asset
![ParkourDataAsset](https://github.com/user-attachments/assets/6425bb58-02fa-45bc-b917-e18a1a2c2ee5)

내부 로직으로 어떤 파쿠르 행동을 할 지 정했다면, 파쿠르를 실제 동작하기 위한 정보들을 담고 있는 **DataAsset.** <br />

파쿠르를 실행할 때 **어떤 애니메이션 몽타주를 사용할 것인지, 어떤 상태로 진입하여 어떤 상태로 남을 것인지**, 애니메이션 몽타주의 실행 위치를 <br />
**Top, Depth, Vault, Mantle**로 상세하게 나눠 값을 지정할 수 있다. <br />

> * Idle / Walk / Sprint 상태별로 작성할 수 있다. <br />
> * 해당 벽의 위치를 기준으로 해당 입력한 값을 더하는 형식으로 위치를 이동시킬 수 있다. <br />
> * **Motion Warping** 기능을 사용한다. <br />

| 문서 | 링크 |
| ------ | ------ |
|Parkour Data Asset | [상세정보](https://www.notion.so/Parkour-System-C-262ca40c46968183b8bdc79c3ca08ffc?source=copy_link#262ca40c46968134a4cdc68a8c5add97) |

<br>

## 4. 파쿠르 액션

### ❖ Vault
---
![Vault](https://github.com/user-attachments/assets/c8ac377e-88b3-4a38-bbc5-8101e87982c1) <br />
**[Vault]** : 벽을 넘는 액션

<br>

![ThinVault](https://github.com/user-attachments/assets/01a6bb59-cd0c-4b71-8088-55b8cbcb38de) <br />
**[ThinVault]** : 얇은 벽을 넘는 액션

<br>

![HighVault](https://github.com/user-attachments/assets/badfaf36-c9b6-4b37-97b2-d6a73c85ce43) <br />
**[HightVault]** : 높은 벽을 넘는 액션

<br>

> ![LowSpeedMantle](https://github.com/user-attachments/assets/b0db8876-db17-4938-9554-153728b62b8c) <br />
> **Vault**를 시도할 때 속도가 부족하거나 Idle 상태일 때에는 **Mantle**이 실행된다.

<br>

### ❖ Mantle
---
![Mantle](https://github.com/user-attachments/assets/7264559c-42a8-4d7f-99a7-723194af69e9) <br />
**[Mantle]** : 넘을 수는 없는 벽을 올라가는 액션

<br>

![LowMantle](https://github.com/user-attachments/assets/ef67816a-e8db-4c67-8e69-74d5adcac387) <br />
**[Mantle]** : 넘을 수는 없는 낮은 벽을 올라가는 액션

<br>

### ❖ Climb
---
![BracedClimb](https://github.com/user-attachments/assets/a23ba843-ff0e-44fe-95c0-b199209681a9) <br />
**[Braced Climb]** : 발을 디딜 수 있는 벽에 매달리는 액션

<br>

![BracedClimbMovement](https://github.com/user-attachments/assets/75e99c70-5631-4e26-90c4-78027ea18094) <br />
**[Braced Climb Movement]** : 발을 디딜 수 있는 벽에서 양 옆으로 이동 하는 액션

<br>

![FreeHang](https://github.com/user-attachments/assets/25e433e1-43fb-4882-8827-93ac99d0b633) <br />
**[FreeHang Climb]** : 발을 디딜 수 없는 벽에 매달리는 액션

<br>

![FreeHangMovement](https://github.com/user-attachments/assets/05f23475-9865-4cf0-beca-1e5f4b82083a) <br />
**[FreeHang Climb Movement]** : 발을 디딜 수 없는 벽에서 양 옆으로 이동 하는 액션

<br>

![ClimbUp](https://github.com/user-attachments/assets/b8ab5420-10ba-4e37-902d-42831c725e32) <br />
**[Climb Up]** : 매달린 벽에서 위에 올라갈 수 있는 공간이 있는 경우 올라가는 액션

<br>

### ❖ Hop
---
![Hop](https://github.com/user-attachments/assets/c6eb5005-3284-474b-92b8-a406b4f31a2b)
![NotCornerHop](https://github.com/user-attachments/assets/17371a23-c5ad-4635-a7ba-437ef66dda04) <br />
**[Hop]** : 매달린 상태에서 다른 벽으로 이동하는 액션

<br>

![CornerHop](https://github.com/user-attachments/assets/e5968db0-d3fc-42b2-a448-42c31519ad86) <br />
**[Corner Hop]** : 매달린 벽에서 수직인 벽이 있는경우 이동하는 액션

<br>

## 5. etc.
| 문서 | README |
| ------ | ------ |
| 사용법 | [파쿠르 플러그인 사용법](https://lively-trumpet-bdf.notion.site/262ca40c46968183b8bdc79c3ca08ffc?source=copy_link) |
| 제작서 | [파쿠르 플러그인 제작서](https://lively-trumpet-bdf.notion.site/262ca40c469681ae822be01e1a9e8ce0?source=copy_link) |
