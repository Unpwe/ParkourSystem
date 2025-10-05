# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
**ParkourSystem** is an Unreal Engine 5.5 C++ plugin providing comprehensive parkour mechanics for characters. It implements parkour actions using State Pattern and Strategy Pattern with Motion Warping for flexible animation positioning. The plugin is designed to be easily integrated into any character with minimal blueprint setup.

## Build System
This is an Unreal Engine 5.5 C++ plugin using Unreal's proprietary build system:

- **Project File**: `CreatePlugins.uproject` (UE 5.5)
- **Plugin Definition**: `ParkourSystem.uplugin`
- **Build Configuration**: `ParkourSystem.Build.cs`
- **Build Commands**: Use Unreal Engine Editor's "Build" option or invoke UnrealBuildTool directly
- **Hot Reload**: Supported through UE Editor (Ctrl+Alt+F11)
- **No Makefile/CMake**: Unreal Engine uses UnrealBuildTool (UBT) exclusively

## Core Architecture

### State Machine Design
The plugin uses GameplayTags as the foundation of its state machine. All state transitions flow through `SetParkourState()` in `ParkourComponent.cpp`, which:
1. Validates state transitions
2. Configures collision, movement mode, and rotation rates
3. Triggers state-specific callbacks
4. Updates character physics settings

**Critical**: When adding new states, you MUST:
1. Declare tag in `ParkourSystem.h` namespace `ParkourGameplayTags`
2. Define tag in `ParkourSystem.cpp` using `UE_DEFINE_GAMEPLAY_TAG_COMMENT`
3. Add enum entry to `EParkourGameplayTagNames`
4. Extend `SetParkourState()` switch logic with new state configuration

### GameplayTag Categories
- **Actions** (`Parkour.Action.*`): Specific parkour moves (Vault, Mantle, Climb variants, Hop variants)
- **States** (`Parkour.State.*`): Current component state (NotBusy, Climb, Mantle, Vault, ReachLedge, WaitingForAssist, ProvidingAssist)
- **Directions** (`Parkour.Direction.*`): Input direction context (Forward, Backward, Left, Right, combinations)
- **ClimbStyles** (`Parkour.ClimbStyle.*`): Climb type (Braced = foot support, FreeHang = no foot support)
- **Interactions** (`Parkour.Interaction.*`): Multiplayer interaction states (ClimbAssist, Available, InProgress)

All tags are declared in `ParkourSystem.h` and defined in `ParkourSystem.cpp`.

### Component Architecture
- **ParkourComponent** (`ParkourComponent.h/cpp`):
  - Core logic hub (~4500+ lines)
  - Handles all parkour calculations, state transitions, wall detection, IK positioning
  - Tick-based movement updates when in Climb state
  - Strategy Pattern: Selects appropriate parkour action based on wall height/depth/width and character velocity

- **ParkourPDA** (`ParkourPDA.h/cpp`):
  - Data-driven configuration system
  - Maps `EParkourGameplayTagNames` to animation montages, state transitions, and Motion Warping parameters
  - Separate configs for Idle/Walk/Sprint character states
  - Configures warp targets (Top/Depth/Vault/Mantle positions relative to detected wall geometry)

- **ParkourAnimInstance** (`ParkourAnimInstance.h/cpp`):
  - Animation Blueprint integration
  - Reads state/action tags from ParkourComponent
  - Manages IK targets for hands and feet during climbing

- **PSFunctionLibrary** (`PSFunctionLibrary.h/cpp`):
  - Static utility functions
  - GameplayTag comparison helpers
  - Common geometric calculations

### Multiplayer Cooperative Climbing System
A two-layer multiplayer assistance system allows Player 1 to request help when unable to climb a wall alone, and Player 2 to provide assistance. The system features both **Manual (explicit)** and **Automatic (implicit)** detection modes.

**New States**:
- `Parkour_State_WaitingForAssist`: P1 is hanging and waiting for help (10-second timeout)
- `Parkour_State_ProvidingAssist`: P2 is actively assisting P1

**Architecture - Two Layers**:

**Layer 1: Automatic Detection (Passive)**
- `CheckForAssistanceOpportunities()`: Called every 0.2 seconds in `TickComponent()`
- `IsPlayerInNeedOfAssistance()`: Fast filtering (5 checks: nullptr, state, already assisted, interaction tag, distance)
- Automatically detects nearby players needing help without manual button press
- Performance optimized: 0.2s cooldown instead of every tick (60 FPS → 5 FPS checks)
- Ready for UI hint integration (e.g., "Press F to assist" prompt)

**Layer 2: Manual Execution (Active)**
- `RequestAssistanceCallable()`: P1 initiates SOS signal, enters WaitingForAssist state
- `ProvideAssistanceCallable(UParkourComponent*)`: P2 accepts assist request, starts sequence
- `CancelAssistanceCallable()`: Either player cancels cooperation
- `ValidateAssistanceRequest()`: Detailed validation (distance, height difference, geometry)
- `StartAssistanceSequence()` / `CompleteAssistanceSequence()`: Manage assistance lifecycle
- `OnAssistanceTimeout()` → `HandleAssistanceTimeout()`: Smart timeout with scenario analysis
- `OnAssistanceComplete()`: Timer callback for 3-second duration completion

**Helper Functions**:
- `FindNearbyPlayersForAssistance()`: Scans world using `TActorIterator` to find eligible helpers
- `IsWithinAssistanceRange()`: Quick distance check (≤300cm)
- `CalculateDistanceToPlayer()`: 3D distance calculation
- `InitializeMultiplayerAssistance()`: Called during `SetInitializeReference()`

**Configuration** (editable in blueprint):
- `MaxAssistanceDistance = 300.f` (cm)
- `AssistanceWaitTimeout = 10.f` (seconds)
- `AssistanceDuration = 3.f` (seconds)
- `AssistanceHeightDifference = 50.f` (cm)

**Function Comparison**:

| Function | Frequency | Purpose | Cost |
|----------|-----------|---------|------|
| `IsPlayerInNeedOfAssistance()` | Every 0.2s | Fast filtering | Low (5 simple checks) |
| `ValidateAssistanceRequest()` | Button click only | Detailed validation | High (geometry, height checks) |

**Delegation Pattern**:
```
OnAssistanceTimeout() (UFUNCTION timer callback)
    ↓
HandleAssistanceTimeout() (business logic with scenario analysis)
```

**Timeout Scenarios** (handled by `HandleAssistanceTimeout()`):
1. No players nearby → Return to Climb state
2. Players nearby but didn't assist → Return to Climb state
3. (Future) Sequence interrupted → Retry with 5s extension
4. (Future) Network issue → Force cleanup

**Important**: Current implementation is **local multiplayer only** (no replication/RPC). For networked multiplayer:
- Add `UPROPERTY(Replicated)` to assistance state variables
- Implement `Server_RequestAssistance()` / `Client_ShowAssistanceUI()` RPCs
- Add `GetLifetimeReplicatedProps()` override
- Implement client prediction + server reconciliation

### Character Integration Pattern
Characters must perform these setup steps (typically in `BeginPlay`):

1. **Add ParkourComponent** to character
2. **Call `SetInitializeReference()`** with:
   - Character mesh
   - Capsule component
   - Character movement component
   - Custom trace channel (created in Project Settings > Collision)
3. **Bind Enhanced Input actions**:
   - Movement input → `MovementInputCallable()` (Triggered) / `ResetMovementCallable()` (Completed)
   - Parkour action key → `PlayParkourCallable()` (Pressed)
   - Drop action → `ParkourDropCallable()` (Pressed, only works during Climb)
   - Cancel action → `ParkourCancelCallable()` (Pressed)
4. **Configure ParkourPDA assets**: Create data assets for each parkour action with appropriate animation montages and warp settings

### Motion Warping Integration
The plugin heavily relies on Motion Warping plugin to adjust character position during parkour animations:
- Wall detection calculates precise warp targets (top edge location, depth offset)
- ParkourPDA assets define additive offsets for each action type
- Warp targets are set before playing animation montages
- Allows same animation to work on walls of varying heights/depths

### IK System
Dynamic IK adjustments during climbing:
- **Hand IK**: Positions hands on wall edges using socket locations (`ik_hand_l`, `ik_hand_r`)
- **Foot IK**: Positions feet relative to hand positions for Braced vs FreeHang styles
- **Socket Names** (configurable):
  - Hands: `hand_l`, `hand_r`, `ik_hand_l`, `ik_hand_r`
  - Feet: `ik_foot_l`, `ik_foot_r`
- **Braced vs FreeHang Detection**: Traces from hand positions downward to detect foot support surfaces

### Debug Features
Debug compilation flags in `ParkourComponent.h` (uncomment to enable):
```cpp
//#define DEBUG_PARKOURCOMPONENT  // General debug logs
//#define DEBUG_TICK              // Tick function logs
//#define DEBUG_MOVEMENT          // Movement input logs
//#define DEBUG_IK                // IK calculation logs
```

Runtime debug draw options (set in blueprint):
- `DDT_ParkourCheckWallShape`: Visualize wall height/width detection
- `DDT_ParkourCheckWallTopDepthShape`: Visualize depth detection
- `DDT_ClimbLedgeResultInspection`: Visualize climb surface validation
- `DDT_CheckInGround`: Visualize ground detection for auto-climb
- `DDT_CheckMantleSurface` / `DDT_CheckClimbSurface` / `DDT_CheckVaultSurface`: Visualize action-specific traces

### Key Configuration Properties
**Auto-Behavior**:
- `bAutoJump`: Automatically perform parkour when approaching obstacles (no button press needed)
- `bAlwaysParkour`: Check parkour conditions every tick vs on-demand via `PlayParkourCallable()`
- `bAutoClimb`: Automatically grab ledges when falling near walls

**Speed Thresholds**:
- `SprintTypeParkour_Speed = 400.f`: Velocity threshold for sprint parkour variants
- `WalkingTypeParkour_Speed = 300.f`: Velocity threshold for walking parkour variants
- Below walking speed uses "Idle" parkour variants (slower, more cautious animations)

**Climb Configuration**:
- `CanClimbHeight = 200.f`: Minimum wall height to enter climb state
- `CheckClimbBracedStyle = -125.f`: Vertical offset to check for foot support (Braced vs FreeHang)
- `bUseCornerHop = true`: Enable corner-detection hop transitions

## File Structure
```
Source/ParkourSystem/
├── Public/
│   ├── ParkourComponent/       # Core component and helper actors
│   ├── DataAssets/            # ParkourPDA definition
│   ├── Animations/            # Animation instances and IK
│   ├── Character/             # Character integration helpers
│   ├── FunctionLibrary/       # PSFunctionLibrary utilities
│   └── ParkourSystem.h        # Module definition, GameplayTag declarations
├── Private/
│   └── [Mirror structure]     # .cpp implementations
├── ParkourSystem.Build.cs     # Module dependencies
└── CLAUDE.md                  # This file
```

## Working with This Codebase

### Adding New Parkour Actions
1. **Declare tags** in `ParkourSystem.h`:
   ```cpp
   UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Action_NewAction);
   ```
2. **Define tag** in `ParkourSystem.cpp`:
   ```cpp
   UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parkour_Action_NewAction, "Parkour.Action.NewAction", "Description");
   ```
3. **Add enum entry** in `ParkourSystem.h` → `EParkourGameplayTagNames`
4. **Create ParkourPDA asset** with animation montage and warp settings
5. **Add action logic** in `ParkourComponent.cpp` (typically in `PlayParkourCallable()` or state-specific update functions)

### Adding New States
1. Follow same tag declaration/definition steps as actions
2. **Extend `SetParkourState()`** in `ParkourComponent.cpp` with new state configuration:
   ```cpp
   else if (UPSFunctionLibrary::CompGameplayTagName(ParkourStateTag, TEXT("Parkour.State.NewState")))
   {
       ParkourStateSettings(CollisionMode, MovementMode, RotationRate, bOrientToMovement, bFlyingMode);
   }
   ```

### Modifying Behavior
- **Animation adjustments**: Edit ParkourPDA assets (no code changes needed)
- **Distances/thresholds**: Modify UPROPERTY values in ParkourComponent.h or expose to blueprints
- **State transitions**: Modify logic in `SetParkourState()` or action-specific functions
- **IK adjustments**: Tune socket offsets and rotation additives in InitializeValues category

### Platform Support
- Currently configured for **Win64** only (see `ParkourSystem.uplugin`)
- Uses Unreal Engine 5.5
- Motion Warping plugin dependency (must be enabled in project)

## Code Documentation Style

### Korean Comments
This plugin includes extensive Korean comments throughout the implementation:

**Function-level documentation** (`ParkourComponent.cpp`):
- All 13 multiplayer assistance functions have detailed Korean block comments
- Explains: 역할 (role), 작동 과정 (operation), 예시 상황 (examples), 결과 (results)
- Written in conversational style ("누군가에게 설명하듯이")

**Inline comments** (`ParkourComponent.h`):
- All multiplayer function declarations have concise inline comments
- All multiplayer variables have descriptive inline comments
- Examples:
  ```cpp
  void RequestAssistanceCallable(); // "도와줘!" SOS 신호 발신
  float MaxAssistanceDistance = 300.f; // 도움 가능 거리 (cm)
  UParkourComponent* AssistingPlayer; // 나를 도와주는 플레이어
  ```

**External documentation**:
- README.md with visual examples and setup instructions
- External Notion documentation (see README.md links)

### Comment Philosophy
- **Header files**: Short, concise inline comments for quick reference
- **Implementation files**: Detailed block comments explaining logic and scenarios
- **Language**: Korean for implementation details, English for API-facing documentation
- **Style**: Practical and clear, focusing on "why" rather than "what"