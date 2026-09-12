void InitializeScene(Engine &engine)
{
	Scene &scene = engine.scene;

	scene.particleRandom = RandomSeed(13);

#define BIND_COMPONENT_POOL(Name, name) \
	ASSERT( OFFSET_OF(Name##Component, entityId) == 0 ); \
	scene.componentPools[ComponentType_##Name] = { \
		.items = (byte*)scene.name##Components, \
		.stride = sizeof(Name##Component), \
		.capacity = ARRAY_COUNT(scene.name##Components), \
	}

	BIND_COMPONENT_POOL(Model, model);
	BIND_COMPONENT_POOL(Sprite, sprite);
	BIND_COMPONENT_POOL(Light, light);
	BIND_COMPONENT_POOL(Particles, particles);
	BIND_COMPONENT_POOL(Script, script);

#undef BIND_COMPONENT_POOL

	// A component reflex knows about but nothing bound above would read an empty pool
	for (u32 type = 0; type < ComponentType_Count; ++type)
	{
		const ComponentPool &pool = scene.componentPools[type];
		const ReflexStruct *reflexStruct = ComponentReflexStruct((ComponentType)type);
		ASSERT( pool.items && pool.capacity );
		ASSERT( reflexStruct && pool.stride == reflexStruct->size );
	}

	// A builtin so it survives CleanScene and keeps the same slot every run, which is
	// what lets a saved ParticlesComponent still refer to it.
	{
		const ParticleEffectDesc desc = {
			.id = { BuiltinID_FountainParticleEffect },
			.name = InternString("fountain_fx"),
			// color and size read begin -> end over the particle's life, so this fades out
			.color = { .min = { 1.0f, 1.0f, 1.0f, 1.0f }, .max = { 1.0f, 1.0f, 1.0f, 0.0f } },
			.size = { .min = 1.0f, .max = 0.3f },
			.rate = 20.0f,
			.burstCount = 20,
			.duration = 0.0f, // Emits until stopped
			.loop = 1,
			// The rest are sampled per particle, between min and max
			.lifetime = { .min = 0.5f, .max = 1.5f },
			.speed = { .min = 1.0f, .max = 3.0f },
			.angle = { .min = 60.0f, .max = 120.0f }, // Degrees, a fountain pointing up
			.gravity = { 0.0f, -9.8f },
			.worldSpace = 1,
		};
		CreateParticleEffect(engine, desc);
	}

	{
		const ParticleEffectDesc desc = {
			.id = { BuiltinID_FireParticleEffect },
			.name = InternString("fire_fx"),
			// color and size read begin -> end over the particle's life, so this fades out
			.color = { .min = { 1.0f, 1.0f, 0.0f, 1.0f }, .max = { 1.0f, 0.0f, 0.0f, 0.0f } },
			.size = { .min = 1.0f, .max = 2.0f },
			.rate = 50.0f,
			.burstCount = 20,
			.duration = 0.0f, // Emits until stopped
			.loop = 1,
			// The rest are sampled per particle, between min and max
			.lifetime = { .min = 0.5f, .max = 1.5f },
			.speed = { .min = 0.5f, .max = 1.0f },
			.angle = { .min = 90.0f, .max = 90.0f }, // Degrees, a fountain pointing up
			.spawnExtent = { .x = 0.2f, .y = 0.0f },
			.gravity = { 0.0f, 0.0f },
			.worldSpace = 1,
		};
		CreateParticleEffect(engine, desc);
	}
}


Sprite &GetSprite(ID id)
{
	ASSERT( Valid(id) );
	Sprite &sprite = *((Sprite*)GetObject(id));
	return sprite;
}

// Sprites keep a dense index, unlike textures: the GPU sprite data buffer and the
// parallel animation state array are both addressed by it.
u16 GetSpriteIndex(const Scene &scene, ID id)
{
	const Sprite &sprite = GetSprite(id);
	const u16 index = (u16)(&sprite - scene.sprites);
	ASSERT( index < scene.spriteCount );
	return index;
}

// Appends a sprite and gives it its ID. Null when the array is full.
static Sprite *PushSprite(Scene &scene, const SpriteDesc &desc)
{
	if ( scene.spriteCount == MAX_SPRITES )
	{
		LOG(Warning, "Could not create sprite, the sprite array is full.\n");
		return nullptr;
	}

	const u16 index = (u16)scene.spriteCount++;
	Sprite &sprite = scene.sprites[index];
	sprite = {};
	sprite.desc = desc;
	scene.spriteAnimStates[index] = {}; // The element held another sprite's animation before

	BindID(&sprite.desc.id, &sprite, IDKind_Sprite);

	return &sprite;
}

ID CreateSprite(Engine &engine, const SpriteDesc &desc)
{
	Graphics &gfx = engine.gfx;

	Sprite *sprite = PushSprite(engine.scene, desc);
	if ( !sprite ) {
		return {};
	}

	sprite->desc.frameCount = desc.frameCount > 0 ? desc.frameCount : 1;

	if ( !Valid(sprite->desc.textureId) )
	{
		LOG(Warning, "Sprite <%s> refers to texture ID %u, which does not exist.\n", desc.name, desc.textureId.slot);
		sprite->desc.textureId = gfx.defaultTexture;
	}

	const Texture &tex = GetTexture(sprite->desc.textureId);
	if ( desc.size.x == 0 && desc.size.y == 0 ) {
		sprite->desc.size = tex.size;
	}

	return sprite->desc.id;
}

ID CreateSprite(Engine &engine, const BinSpriteDesc &desc)
{
	const SpriteDesc txtDesc = {
		.id          = desc.id,
		.name        = desc.name,
		.textureId   = desc.textureId,
		.pos         = desc.pos,
		.size        = desc.size,
		.frameCount  = desc.frameCount,
		.fps         = desc.fps,
		.loop        = desc.loop,
	};
	return CreateSprite(engine, txtDesc);
};

ID FindSprite(const Scene &scene, const char *name)
{
	if (!name) return {};
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &desc = scene.sprites[i].desc;
		if (StrEq(desc.name, name)) return desc.id;
	}
	return {};
}

ID FindSprite(const Scene &scene, ID textureId, uint2 pos, uint2 size)
{
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &desc = scene.sprites[i].desc;
		if (desc.textureId == textureId &&
			desc.pos.x == pos.x && desc.pos.y == pos.y &&
			desc.size.x == size.x && desc.size.y == size.y) return desc.id;
	}
	return {};
}

ID GetOrCreateSprite(Engine &engine, const SpriteDesc &desc)
{
	ID id = FindSprite(engine.scene, desc.textureId, desc.pos, desc.size);
	if (!id)
		id = CreateSprite(engine, desc);
	return id;
}

void RemoveSprite(Scene &scene, ID id)
{
	if ( IsBuiltin(id) )
	{
		LOG(Warning, "Ignoring an attempt to remove builtin sprite <%s>.\n", GetSprite(id).desc.name);
		return;
	}

	if (id)
	{
		// Marks only. The sprite keeps its element until CompactSprites, so the tiles
		// and entities still drawing it this frame have something valid to read.
		GetSprite(id).desc.id = {};
		Invalidate(id);
	}
}

static COMPACT_MOVE(MoveSprite)
{
	Scene &scene = *(Scene*)data;
	scene.spriteAnimStates[dstIndex] = scene.spriteAnimStates[srcIndex];
}

void CompactSprites(Scene &scene)
{
	COMPACT_ARRAY_BY_ID(Sprite, scene.sprites, scene.spriteCount, desc.id,
			MoveSprite, nullptr, &scene);
}


////////////////////////////////////////////////////////////////////////
// Particle effect management

ParticleEffect &GetParticleEffect(ID id)
{
	ASSERT( Valid(id) );
	ParticleEffect &effect = *((ParticleEffect*)GetObject(id));
	return effect;
}

static ParticleEffect *PushParticleEffect(Scene &scene, const ParticleEffectDesc &desc)
{
	if ( scene.particleEffectCount == MAX_PARTICLE_EFFECTS )
	{
		LOG(Warning, "Could not create particle effect, the particle effect array is full.\n");
		return nullptr;
	}

	ParticleEffect &effect = scene.particleEffects[scene.particleEffectCount++];
	effect = {};
	effect.desc = desc;

	BindID(&effect.desc.id, &effect, IDKind_ParticleEffect);

	return &effect;
}

ID CreateParticleEffect(Engine &engine, const ParticleEffectDesc &desc)
{
	ParticleEffect *effect = PushParticleEffect(engine.scene, desc);
	if ( !effect ) {
		return {};
	}

	effect->desc.name = InternString(desc.name);

	// An effect with no sprite is a valid work in progress, but one naming a sprite that
	// is not there would silently draw nothing, so it is worth a word.
	if ( desc.spriteID.slot != 0 && !Valid(desc.spriteID) )
	{
		LOG(Warning, "Particle effect <%s> refers to sprite ID %u, which does not exist.\n", desc.name, desc.spriteID.slot);
		effect->desc.spriteID = {};
	}

	return effect->desc.id;
}

ID FindParticleEffect(const Scene &scene, const char *name)
{
	if (!name) return {};
	for (u32 i = 0; i < scene.particleEffectCount; ++i)
	{
		const ParticleEffectDesc &desc = scene.particleEffects[i].desc;
		if (StrEq(desc.name, name)) return desc.id;
	}
	return {};
}

void RemoveParticleEffect(Scene &scene, ID id)
{
	if ( IsBuiltin(id) )
	{
		LOG(Warning, "Ignoring an attempt to remove builtin particle effect <%s>.\n", GetParticleEffect(id).desc.name);
		return;
	}

	if (id)
	{
		GetParticleEffect(id).desc.id = {};
		Invalidate(id);
	}
}

void CompactParticleEffects(Scene &scene)
{
	COMPACT_ARRAY_BY_ID(ParticleEffect, scene.particleEffects, scene.particleEffectCount, desc.id, nullptr, nullptr, nullptr);
}



////////////////////////////////////////////////////////////////////////
// Particle simulation

static void SpawnParticle(Scene &scene, ID entityId, const ParticleEffectDesc &effect)
{
	// Dropping the newest is cheaper than hunting for the oldest, and at the cap
	// nobody can tell the difference
	if ( scene.particleCount == MAX_PARTICLES ) { return; }

	const Entity &entity = GetEntity(entityId);

	const float2 jitter = {
		effect.spawnExtent.x * RandomBipolar(scene.particleRandom),
		effect.spawnExtent.y * RandomBipolar(scene.particleRandom),
	};
	const f32 angle = RandomRange(scene.particleRandom, effect.angle) * ToRadians;
	const f32 speed = RandomRange(scene.particleRandom, effect.speed);
	const float2 origin = effect.worldSpace ? entity.position.xy : float2{};

	Particle &p = scene.particles[scene.particleCount++];
	p = {
		.pos = origin + effect.spawnOffset + jitter,
		.vel = { Cos(angle) * speed, Sin(angle) * speed },
		.age = 0.0f,
		.lifetime = Max(RandomRange(scene.particleRandom, effect.lifetime), 0.0001f), // Divided by at draw time
		.effectId = effect.id,
		// Only local-space particles need to know their emitter
		.entityId = effect.worldSpace ? ID{} : entityId,
	};
}

void SimulateParticles(Scene &scene, f32 deltaSeconds)
{
	// Emission
	const u32 particleComponentCount = scene.componentPools[ComponentType_Particles].count;
	for (u32 i = 0; i < particleComponentCount; ++i)
	{
		ParticlesComponent &particles = scene.particlesComponents[i];
		if ( !particles.playing || !particles.effectId ) { continue; }

		const ParticleEffectDesc &effect = GetParticleEffect(particles.effectId).desc;

		particles.elapsedTime += deltaSeconds;
		if ( effect.duration > 0.0f && particles.elapsedTime >= effect.duration )
		{
			if ( effect.loop ) {
				particles.elapsedTime -= effect.duration;
			} else {
				particles.playing = 0; // Already-live particles still finish their lives
				continue;
			}
		}

		// The accumulator is what keeps a rate of 3.5/s from rounding to 3 or 4
		particles.emitAccum += effect.rate * deltaSeconds;
		while ( particles.emitAccum >= 1.0f )
		{
			SpawnParticle(scene, particles.entityId, effect);
			particles.emitAccum -= 1.0f;
		}
	}

	// Integration. Swap-remove keeps the array packed, so the draw pass is a flat walk.
	for (u32 i = 0; i < scene.particleCount; )
	{
		Particle &p = scene.particles[i];
		p.age += deltaSeconds;

		// A dead effect takes its particles with it, and so does a dead emitter for the
		// local-space ones: a slot that was set but no longer resolves
		const bool orphaned = ( p.entityId.slot != 0 && !Valid(p.entityId) );
		if ( p.age >= p.lifetime || !p.effectId || orphaned )
		{
			scene.particles[i] = scene.particles[--scene.particleCount];
			continue;
		}

		const ParticleEffectDesc &effect = GetParticleEffect(p.effectId).desc;
		p.vel += deltaSeconds * effect.gravity;
		p.vel = Max(0.0f, 1.0f - effect.drag * deltaSeconds) * p.vel;
		p.pos += deltaSeconds * p.vel;
		++i;
	}
}

void PlayParticles(Scene &scene, ID entityId)
{
	if ( entityId )
	{
		const Entity &entity = GetEntity(entityId);
		if ( HasComponents(scene, entity.id, Component_Particles) )
		{
			ParticlesComponent &particles = GetParticles(scene, entity.id);
			if ( particles.effectId )
			{
				particles.playing = 1;
				particles.elapsedTime = 0.0f;
				particles.emitAccum = 0.0f;

				const ParticleEffectDesc &effect = GetParticleEffect(particles.effectId).desc;
				for (u32 i = 0; i < effect.burstCount; ++i)
				{
					SpawnParticle(scene, entityId, effect);
				}
			}
		}
	}
}

void StopParticles(Scene &scene, ID entityId)
{
	if ( entityId )
	{
		const Entity &entity = GetEntity(entityId);
		if ( HasComponents(scene, entity.id, Component_Particles) )
		{
			ParticlesComponent &particles = GetParticles(scene, entity.id);
			particles.playing = 0;
		}
	}
}

void StartParticles(Scene &scene)
{
	const u32 particleComponentCount = scene.componentPools[ComponentType_Particles].count;
    for (u32 i = 0; i < particleComponentCount; ++i)
    {
        if ( scene.particlesComponents[i].playOnStart ) {
            PlayParticles(scene, scene.particlesComponents[i].entityId);
        }
    }
}

void ClearParticles(Scene &scene)
{
	scene.particleCount = 0;

	const u32 particleComponentCount = scene.componentPools[ComponentType_Particles].count;
	for (u32 i = 0; i < particleComponentCount; ++i)
	{
		ParticlesComponent &particles = scene.particlesComponents[i];
		particles.playing = 0;
		particles.emitAccum = 0.0f;
		particles.elapsedTime = 0.0f;
	}
}



////////////////////////////////////////////////////////////////////////
// Room management

Room &GetRoom(ID id)
{
	ASSERT( Valid(id) );
	Room &room = *((Room*)GetObject(id));
	return room;
}

u16 GetRoomIndex(const Scene &scene, ID id)
{
	const Room &room = GetRoom(id);
	const u16 index = (u16)(&room - scene.rooms);
	ASSERT( index < scene.roomCount );
	return index;
}

static COMPACT_MOVE(MoveRoom)
{
	Scene &scene = *(Scene*)data;
	Room &room = scene.rooms[dstIndex];

	for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
	{
		if (room.layers[i].initialized) {
			SetObject(room.layers[i].id, &room.layers[i]);
		}
	}
}

void CompactRooms(Scene &scene)
{
	COMPACT_ARRAY_BY_ID(Room, scene.rooms, scene.roomCount, id, MoveRoom, nullptr, &scene);
}


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(ID entityId)
{
	ASSERT( Valid(entityId) );
	Entity &entity = *((Entity*)GetObject(entityId));
	return entity;
}

// Entities keep a dense index, unlike textures: the GPU entity buffer is addressed by it.
u16 GetEntityIndex(const Scene &scene, ID entityId)
{
	const Entity &entity = GetEntity(entityId);
	const u16 index = (u16)(&entity - scene.entities);
	ASSERT( index < scene.entityCount );
	return index;
}

bool HasComponents(const Scene &scene, ID entityId, ComponentFlags components)
{
	if ( !Valid(entityId) ) {
		return false;
	}
	const u16 index = GetEntityIndex(scene, entityId);
	for (u32 type = 0; type < ComponentType_Count; ++type)
	{
		const bool wanted = ( components & (1 << type) ) != 0;
		if ( wanted && scene.entityComponentIndex[index][type] == NO_COMPONENT ) {
			return false;
		}
	}
	return true;
}

static void *ComponentAt(const ComponentPool &pool, u32 index)
{
	void *component = pool.items + index * pool.stride;
	return component;
}

void *AddComponentSlot(Scene &scene, ID entityId, ComponentType componentType)
{
	ComponentPool &pool = scene.componentPools[componentType];
	const u16 entityIndex = GetEntityIndex(scene, entityId);

	u16 slot = scene.entityComponentIndex[entityIndex][componentType];
	if ( slot == NO_COMPONENT )
	{
		if (pool.count == pool.capacity) {
			LOG(Warning, "Could not add a <%s> component, its pool is full.\n", ComponentNames[componentType]);
			return nullptr;
		}

		slot = (u16)pool.count++;
		scene.entityComponentIndex[entityIndex][componentType] = slot;
	}

	void *component = ComponentAt(pool, slot);
	MemSet(component, pool.stride, 0);
	*(ID*)component = entityId;
	return component;
}

void RemoveComponentSlot(Scene &scene, ID entityId, ComponentType componentType)
{
	ComponentPool &pool = scene.componentPools[componentType];
	const u16 entityIndex = GetEntityIndex(scene, entityId);

	const u16 slot = scene.entityComponentIndex[entityIndex][componentType];
	if (slot == NO_COMPONENT) {
		return;
	}

	const u16 last = (u16)(--pool.count);
	if (slot != last)
	{
		void *moved = ComponentAt(pool, slot);
		MemCopy(moved, ComponentAt(pool, last), pool.stride);
		const ID movedEntityId = *(const ID*)moved;
		scene.entityComponentIndex[ GetEntityIndex(scene, movedEntityId) ][componentType] = slot;
	}

	scene.entityComponentIndex[entityIndex][componentType] = NO_COMPONENT;
}

void *GetComponentSlot(const Scene &scene, ID entityId, ComponentType componentType)
{
	const u16 slot = scene.entityComponentIndex[GetEntityIndex(scene, entityId)][componentType];
	ASSERT( slot != NO_COMPONENT );
	void *component = ComponentAt(scene.componentPools[componentType], slot);
	return component;
}

static void UpdateModelGeometry(Graphics &gfx, ModelComponent &model)
{
	model.vertices = GetVerticesForGeometryType(gfx, model.geometryType);
	model.indices = GetIndicesForGeometryType(gfx, model.geometryType);
}

static ModelComponent *AddModel(Engine &engine, ID entityId)
{
	ModelComponent *model = (ModelComponent*)AddComponentSlot(engine.scene, entityId, ComponentType_Model);
	if ( model )
	{
		model->materialId = engine.gfx.defaultMaterial;
		model->geometryType = GeometryTypeCube;
		UpdateModelGeometry(engine.gfx, *model);
	}
	return model;
}

ModelComponent &GetModel(Scene &scene, ID entityId)
{
	ModelComponent &model = *(ModelComponent*)GetComponentSlot(scene, entityId, ComponentType_Model);
	return model;
}

const ModelComponent &GetModel(const Scene &scene, ID entityId)
{
	const ModelComponent &model = *(const ModelComponent*)GetComponentSlot(scene, entityId, ComponentType_Model);
	return model;
}

void SetModelGeometryType(Engine &engine, ModelComponent &model, GeometryType geometryType)
{
	model.geometryType = geometryType;
	UpdateModelGeometry(engine.gfx, model);
}

static SpriteComponent *AddSpriteComponent(Scene &scene, ID entityId)
{
	SpriteComponent *sprite = (SpriteComponent*)AddComponentSlot(scene, entityId, ComponentType_Sprite);
	return sprite;
}

SpriteComponent &GetSprite(Scene &scene, ID entityId)
{
	SpriteComponent &sprite = *(SpriteComponent*)GetComponentSlot(scene, entityId, ComponentType_Sprite);
	return sprite;
}

const SpriteComponent &GetSprite(const Scene &scene, ID entityId)
{
	const SpriteComponent &sprite = *(const SpriteComponent*)GetComponentSlot(scene, entityId, ComponentType_Sprite);
	return sprite;
}

// Most callers only want the ID and do not care whether the entity has the component,
// so these answer with a null ID instead of making every site test for it first
ID EntityMaterialId(const Scene &scene, ID entityId)
{
	if ( !HasComponents(scene, entityId, Component_Model) ) {
		return {};
	}
	return GetModel(scene, entityId).materialId;
}

ID EntitySpriteId(const Scene &scene, ID entityId)
{
	if ( !HasComponents(scene, entityId, Component_Sprite) ) {
		return {};
	}
	return GetSprite(scene, entityId).spriteId;
}

ID EntityLayerId(const Scene &scene, ID entityId)
{
	if ( !HasComponents(scene, entityId, Component_Sprite) ) {
		return {};
	}
	return GetSprite(scene, entityId).layerId;
}

static LightComponent *AddLight(Scene &scene, ID entityId)
{
	LightComponent *light = (LightComponent*)AddComponentSlot(scene, entityId, ComponentType_Light);
	if ( light )
	{
		light->type = LightType_Point;
		light->color = Float3(1.0f);
		light->intensity = 2.0f;
		light->radius = 5.0f;
	}
	return light;
}

LightComponent &GetLight(Scene &scene, ID entityId)
{
	LightComponent &light = *(LightComponent*)GetComponentSlot(scene, entityId, ComponentType_Light);
	return light;
}

const LightComponent &GetLight(const Scene &scene, ID entityId)
{
	const LightComponent &light = *(const LightComponent*)GetComponentSlot(scene, entityId, ComponentType_Light);
	return light;
}

static ParticlesComponent *AddParticles(Scene &scene, ID entityId)
{
	ParticlesComponent *particles = (ParticlesComponent*)AddComponentSlot(scene, entityId, ComponentType_Particles);
	if ( particles )
	{
		particles->effectId = { BuiltinID_FountainParticleEffect };
		particles->playOnStart = 1;
	}
	return particles;
}

ParticlesComponent &GetParticles(Scene &scene, ID entityId)
{
	ParticlesComponent &particles = *(ParticlesComponent*)GetComponentSlot(scene, entityId, ComponentType_Particles);
	return particles;
}

const ParticlesComponent &GetParticles(const Scene &scene, ID entityId)
{
	const ParticlesComponent &particles = *(const ParticlesComponent*)GetComponentSlot(scene, entityId, ComponentType_Particles);
	return particles;
}

ScriptComponent &GetScript(Scene &scene, ID entityId)
{
	ScriptComponent &script = *(ScriptComponent*)GetComponentSlot(scene, entityId, ComponentType_Script);
	return script;
}

const ScriptComponent &GetScript(const Scene &scene, ID entityId)
{
	const ScriptComponent &script = *(const ScriptComponent*)GetComponentSlot(scene, entityId, ComponentType_Script);
	return script;
}

// The high half indexes the GPU entity buffer, the low half is the entity's ID slot.
// The shaders pull the index back out with a >>16 and compare the whole value against
// globals.selectedEntity, so both halves have to stay where they are.
CT_ASSERT(ILU_ID_MAX_SLOTS <= U16_MAX);

u32 EntityDrawId(const Scene &scene, ID entityId)
{
	const u32 index = GetEntityIndex(scene, entityId);
	const u32 drawId = (index << 16) | entityId.slot;
	return drawId;
}

ID EntityFromDrawId(u32 drawId)
{
	// The readback is a frame behind, but slots are never recycled, so a stale one
	// either still names the same entity or has gone invalid for good. Slot 0 is the
	// background, and never resolves.
	const ID entityId = { .slot = drawId & 0xFFFF };
	return Valid(entityId) ? entityId : ID{};
}

static COMPACT_MOVE(MoveEntity)
{
	Scene &scene = *(Scene*)data;
	for (u32 type = 0; type < ComponentType_Count; ++type) {
		scene.entityComponentIndex[dstIndex][type] = scene.entityComponentIndex[srcIndex][type];
	}
}

static COMPACT_REMOVE(ClearEntityComponents)
{
	Scene &scene = *(Scene*)data;
	for (u32 type = 0; type < ComponentType_Count; ++type) {
		scene.entityComponentIndex[index][type] = NO_COMPONENT;
	}
}

void CompactEntities(Scene &scene)
{
	COMPACT_ARRAY_BY_ID(Entity, scene.entities, scene.entityCount, id, MoveEntity, ClearEntityComponents, &scene);
}

void EntitySetPosition(Entity &entity, float3 position)
{
	entity.position = position;
}

ComponentDesc *PushComponentDesc(ComponentDescPool &pool, u32 entityIndex, ComponentType type)
{
	if ( pool.componentCount == pool.componentCapacity ) {
		LOG(Warning, "Could not push a <%s> component descriptor, the pool is full.\n", ComponentNames[type]);
		return nullptr;
	}

	ComponentDesc &desc = pool.components[pool.componentCount++];
	desc = {};
	desc.entityIndex = entityIndex;
	desc.type = type;
	return &desc;
}

// A component and the descriptor reflex generates for it hold the same properties in the same
// order, only at different offsets, so the copy walks both member lists at once
static void CopyProperties(const ReflexStruct &dstType, void *dst, const ReflexStruct &srcType, const void *src)
{
	ASSERT( dstType.memberCount == srcType.memberCount );

	for (u32 i = 0; i < dstType.memberCount; ++i)
	{
		const ReflexMember &dstMember = dstType.members[i];
		const ReflexMember &srcMember = srcType.members[i];
		ASSERT( dstMember.reflexId == srcMember.reflexId );

		MemCopy((byte*)dst + dstMember.offset, (const byte*)src + srcMember.offset, ReflexGetTypeSize(dstMember.reflexId));
	}
}

static void MakeComponentDesc(ComponentType type, const void *component, void *desc)
{
	const ReflexStruct *componentType = ComponentReflexStruct(type);
	const ReflexStruct *descType = ComponentDescReflexStruct(type);

	if ( !componentType || !descType ) {
		LOG(Warning, "Component <%s> is not reflected, its descriptor stays empty.\n", ComponentNames[type]);
		return;
	}

	CopyProperties(*descType, desc, *componentType, component);
}

static void ApplyComponentDesc(ComponentType type, void *component, const void *desc)
{
	const ReflexStruct *componentType = ComponentReflexStruct(type);
	const ReflexStruct *descType = ComponentDescReflexStruct(type);

	if ( !componentType || !descType ) {
		LOG(Warning, "Component <%s> is not reflected, its descriptor is not applied.\n", ComponentNames[type]);
		return;
	}

	CopyProperties(*componentType, component, *descType, desc);
}

void GatherEntityComponentDescs(Engine &engine, ID entityId, u32 entityIndex, ComponentDescPool &pool)
{
	Scene &scene = engine.scene;

	for (u32 type = 0; type < ComponentType_Count; ++type)
	{
		const ComponentFlags bit = 1 << type;

		if ( HasComponents(scene, entityId, bit) ) {

			if ( type == ComponentType_Script )
			{
				const ScriptComponent &script = GetScript(scene, entityId);
				if ( script.name )
				{
					if ( ComponentDesc *desc = PushComponentDesc(pool, entityIndex, ComponentType_Script) ) {
						desc->script = MakeDesc(script, pool);
					}
				}
			}
			else if ( ComponentDesc *desc = PushComponentDesc(pool, entityIndex, (ComponentType)type) )
			{
				MakeComponentDesc((ComponentType)type, GetComponentSlot(scene, entityId, (ComponentType)type), ComponentDescData(*desc));
			}
		}
	}
}

void AddComponent(Engine &engine, ID entityId, ComponentType type)
{
	if ( !Valid(entityId) ) {
		LOG(Warning, "AddComponent: entity ID %u does not exist.\n", entityId.slot);
		return;
	}

	switch ( type )
	{
		case ComponentType_Model:
			AddModel(engine, entityId);
			break;

		case ComponentType_Sprite:
			AddSpriteComponent(engine.scene, entityId);
			break;

		case ComponentType_Light:
			AddLight(engine.scene, entityId);
			break;

		case ComponentType_Particles:
			AddParticles(engine.scene, entityId);
			break;

		case ComponentType_Script:
			AddScript(engine, entityId);
			break;

		default:
			LOG(Warning, "Ignoring an attempt to add a component of unknown type %u.\n", type);
			break;
	}
}

void RemoveComponent(Engine &engine, ID entityId, ComponentType componentType)
{
	if ( !entityId ) {
		LOG(Warning, "Ignoring an attempt to remove a component from an invalid entity\n");
		return;
	}

	if ( componentType >= ComponentType_Count ) {
		LOG(Warning, "Ignoring an attempt to remove a component of unknown type: %u\n", componentType);
		return;
	}

	if ( componentType == ComponentType_Script ) {
		RemoveScript(engine, entityId);
	} else {
		RemoveComponentSlot(engine.scene, entityId, componentType);
	}
}

void AddComponent(Engine &engine, ID entityId, const ComponentDesc &desc)
{
	if ( !Valid(entityId) ) {
		LOG(Warning, "AddComponent: entity ID %u does not exist.\n", entityId.slot);
		return;
	}

	if ( desc.type == ComponentType_Script ) {
		AddScript(engine, entityId, desc.script);
		return;
	}

	if ( desc.type >= ComponentType_Count ) {
		LOG(Warning, "Ignoring a component descriptor of unknown type %u.\n", desc.type);
		return;
	}

	AddComponent(engine, entityId, desc.type);

	// Its pool was full, and AddComponent already said so
	if ( !HasComponents(engine.scene, entityId, (ComponentFlags)(1 << desc.type)) ) {
		return;
	}

	void *component = GetComponentSlot(engine.scene, entityId, desc.type);

	ApplyComponentDesc(desc.type, component, ComponentDescData(desc));

	// What the components keep beyond their properties. The IDs saved in a scene are only
	// checked here, where the entity can still be named in the warning.
	switch ( desc.type )
	{
		case ComponentType_Model:
		{
			ModelComponent &model = *(ModelComponent*)component;

			if ( desc.model.materialId.slot != 0 && !Valid(desc.model.materialId) )
			{
				LOG(Warning, "Entity <%s> refers to material ID %u, which does not exist.\n", GetEntity(entityId).name, desc.model.materialId.slot);
				model.materialId = engine.gfx.defaultMaterial;
			}

			UpdateModelGeometry(engine.gfx, model);
			break;
		}

		case ComponentType_Sprite:
		{
			SpriteComponent &sprite = *(SpriteComponent*)component;

			if ( desc.sprite.spriteId.slot != 0 && !Valid(desc.sprite.spriteId) )
			{
				LOG(Warning, "Entity <%s> refers to sprite ID %u, which does not exist.\n", GetEntity(entityId).name, desc.sprite.spriteId.slot);
				sprite.spriteId = {};
			}
			break;
		}

		default:;
	}
}

EntityDesc GetEntityDesc(Engine &engine, ID entityId)
{
	const Entity &entity = GetEntity(entityId);
	EntityDesc entityDesc = {
		.id      = entity.id,
		.name    = entity.name,
		.pos     = entity.position,
		.scale   = entity.scale,
	};
	return entityDesc;
}

// Appends an entity and gives it its ID. Null when the array is full. Entities do not
// keep their descriptor around, so the ID comes in on its own.
static Entity *PushEntity(Scene &scene, ID entityId)
{
	if ( scene.entityCount == MAX_ENTITIES )
	{
		LOG(Warning, "Could not create entity, the entity array is full.\n");
		return nullptr;
	}

	const u32 index = scene.entityCount++;
	Entity &entity = scene.entities[index];
	entity = { .id = entityId };

	for (u32 type = 0; type < ComponentType_Count; ++type) {
		scene.entityComponentIndex[index][type] = NO_COMPONENT;
	}

	BindID(&entity.id, &entity, IDKind_Entity);

	return &entity;
}

ID CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Entity *entity = PushEntity(engine.scene, desc.id);
	if ( !entity ) {
		return {};
	}

	entity->name = desc.name;
	entity->visible = true;
	EntitySetPosition(*entity, desc.pos);
	entity->scale = desc.scale;

	return entity->id;
}

static EntityDesc EntityDescFromBin(const BinEntityDesc &desc, u32 entityIndex, ComponentDescPool &pool)
{
	EntityDesc entityDesc = {
		.id = desc.id,
		.name = desc.name,
		.pos = desc.pos,
		.scale = desc.scale,
	};

	if ( desc.components & Component_Model )
	{
		if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Model) ) {
			component->model.materialId = desc.materialId;
			component->model.geometryType = desc.geometryType;
		}
	}

	if ( desc.components & Component_Sprite )
	{
		if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Sprite) ) {
			component->sprite.spriteId = desc.spriteId;
			component->sprite.layerId = desc.layerId;
		}
	}

	if ( desc.components & Component_Light )
	{
		if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Light) ) {
			component->light = desc.light;
		}
	}

	if ( desc.components & Component_Particles )
	{
		if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Particles) ) {
			component->particles = desc.particles;
		}
	}

	if ( desc.components & Component_Script )
	{
		const BinScriptDesc &binScript = desc.script;
		const u32 propertyCount = Min(binScript.propertyCount, (u32)ARRAY_COUNT(binScript.properties));

		if ( pool.propertyCount + propertyCount > pool.propertyCapacity )
		{
			LOG(Warning, "Entity <%s> drops its script properties, the property pool is full.\n", desc.name);
		}
		else if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Script) )
		{
			ScriptComponentDesc &script = component->script;
			script.name = binScript.name;
			script.properties = pool.properties + pool.propertyCount;
			script.propertyCount = propertyCount;

			for (u32 p = 0; p < propertyCount; ++p)
			{
				const BinScriptPropertyDesc &binProperty = binScript.properties[p];

				ScriptPropertyDesc &property = pool.properties[pool.propertyCount++];
				property.name = binProperty.name;
				property.type = binProperty.type;
				MemCopy(property.value, binProperty.value, sizeof(property.value));
			}
		}
	}

	return entityDesc;
}

ID CreateEntity(Engine &engine, const BinEntityDesc &desc)
{
	ComponentDesc components[ComponentType_Count] = {};
	ScriptPropertyDesc properties[MAX_SCRIPT_PROPERTIES] = {};
	ComponentDescPool pool = {
		.components = components,
		.componentCapacity = ARRAY_COUNT(components),
		.properties = properties,
		.propertyCapacity = ARRAY_COUNT(properties),
	};

	const EntityDesc entityDesc = EntityDescFromBin(desc, 0, pool);

	const ID entityId = CreateEntity(engine, entityDesc);
	for (u32 i = 0; i < pool.componentCount; ++i) {
		AddComponent(engine, entityId, pool.components[i]);
	}
	return entityId;
}

void RemoveEntity(Engine &engine, ID id)
{
	if (id)
	{
		for (u32 type = 0; type < ComponentType_Count; ++type) {
			RemoveComponent(engine, id, (ComponentType)type);
		}

		GetEntity(id).id = {};
		Invalidate(id);
	}
}

ID DuplicateEntity(Engine &engine, ID entityId)
{
	ComponentDesc components[ComponentType_Count] = {};
	ScriptPropertyDesc properties[MAX_SCRIPT_PROPERTIES] = {};
	ComponentDescPool pool = {
		.components = components,
		.componentCapacity = ARRAY_COUNT(components),
		.properties = properties,
		.propertyCapacity = ARRAY_COUNT(properties),
	};

	EntityDesc desc = GetEntityDesc(engine, entityId);
	GatherEntityComponentDescs(engine, entityId, 0, pool);

	desc.id = {}; // The copy is a new entity, so let the pool hand it its own ID

	const ID copyId = CreateEntity(engine, desc);
	for (u32 i = 0; i < pool.componentCount; ++i) {
		AddComponent(engine, copyId, pool.components[i]);
	}
	return copyId;
}


////////////////////////////////////////////////////////////////////////
// Prefab management

Prefab &GetPrefab(ID id)
{
	ASSERT( Valid(id) );
	Prefab &prefab = *((Prefab*)GetObject(id));
	return prefab;
}

u16 GetPrefabIndex(const Scene &scene, ID id)
{
	const Prefab &prefab = GetPrefab(id);
	const u16 index = (u16)(&prefab - scene.prefabs);
	ASSERT( index < scene.prefabCount );
	return index;
}

ID FindPrefab(const Scene &scene, const char *name)
{
	if (!name) return {};
	for (u32 i = 0; i < scene.prefabCount; ++i)
	{
		if (StrEq(scene.prefabs[i].name, name)) return scene.prefabs[i].id;
	}
	return {};
}

// Appends a prefab and gives it its ID. Null when the array is full. Prefabs do not
// keep their descriptor around, so the ID comes in on its own.
static Prefab *PushPrefab(Scene &scene, ID id)
{
	if ( scene.prefabCount == MAX_PREFABS )
	{
		LOG(Warning, "Could not create prefab, the prefab array is full.\n");
		return nullptr;
	}

	Prefab &prefab = scene.prefabs[scene.prefabCount++];
	prefab = { .id = id };

	BindID(&prefab.id, &prefab, IDKind_Prefab);

	return &prefab;
}

ID CreatePrefab(Engine &engine, const PrefabDesc &desc)
{
	if ( desc.entityCount > MAX_PREFAB_ENTITIES )
	{
		LOG(Warning, "Could not create prefab <%s>, it has %u entities, more than the %u max.\n", desc.name, desc.entityCount, MAX_PREFAB_ENTITIES);
		return {};
	}

	Prefab *prefabPtr = PushPrefab(engine.scene, desc.id);
	if ( !prefabPtr ) {
		return {};
	}

	Prefab &prefab = *prefabPtr;
	prefab.name = InternString(desc.name);
	prefab.entityCount = desc.entityCount;
	for (u32 i = 0; i < desc.entityCount; ++i) {
		prefab.entities[i] = desc.entities[i];
	}

	prefab.componentCount = Min(desc.componentCount, (u32)ARRAY_COUNT(prefab.components));
	if ( desc.componentCount > prefab.componentCount )
	{
		LOG(Warning, "Prefab <%s> has %u components, only the first %u are kept.\n", desc.name, desc.componentCount, prefab.componentCount);
	}

	// A prefab outlives the descriptors it was built from, so the script properties its
	// components point at are copied into storage the prefab owns
	prefab.scriptPropertyCount = 0;
	for (u32 i = 0; i < prefab.componentCount; ++i)
	{
		ComponentDesc &component = prefab.components[i];
		component = desc.components[i];

		if ( component.type != ComponentType_Script ) {
			continue;
		}

		ScriptComponentDesc &script = component.script;
		const ScriptPropertyDesc *source = script.properties;

		if ( prefab.scriptPropertyCount + script.propertyCount > ARRAY_COUNT(prefab.scriptProperties) )
		{
			LOG(Warning, "Prefab <%s> drops the properties of script <%s>, its property storage is full.\n", desc.name, script.name);
			script.properties = nullptr;
			script.propertyCount = 0;
			continue;
		}

		script.properties = prefab.scriptProperties + prefab.scriptPropertyCount;
		for (u32 p = 0; p < script.propertyCount; ++p) {
			prefab.scriptProperties[prefab.scriptPropertyCount++] = source[p];
		}
	}

	return prefab.id;
}


ID CreatePrefab(Engine &engine, const BinPrefabDesc &desc)
{
	ComponentDesc components[MAX_PREFAB_COMPONENTS] = {};
	ScriptPropertyDesc properties[MAX_PREFAB_SCRIPT_PROPERTIES] = {};
	ComponentDescPool pool = {
		.components = components,
		.componentCapacity = ARRAY_COUNT(components),
		.properties = properties,
		.propertyCapacity = ARRAY_COUNT(properties),
	};

	PrefabDesc prefabDesc = {};
	prefabDesc.id = desc.id;
	prefabDesc.name = desc.name;
	prefabDesc.entityCount = Min(desc.entityCount, (u32)ARRAY_COUNT(prefabDesc.entities));

	for (u32 i = 0; i < prefabDesc.entityCount; ++i)
	{
		prefabDesc.entities[i] = EntityDescFromBin(desc.entities[i], i, pool);
	}

	prefabDesc.components = pool.components;
	prefabDesc.componentCount = pool.componentCount;

	return CreatePrefab(engine, prefabDesc);
}

void RemovePrefab(Scene &scene, ID id)
{
	if (id)
	{
		// Marks only, see RemoveSprite
		GetPrefab(id).id = {};
		Invalidate(id);
	}
}

void CompactPrefabs(Scene &scene)
{
	COMPACT_ARRAY_BY_ID(Prefab, scene.prefabs, scene.prefabCount, id, nullptr, nullptr, nullptr);
}

// Spawns every entity in the prefab, offset by atPosition, as freshly created entities
// unrelated to the template. Returns the first one, since there is no hierarchy yet to
// name a single root.
ID InstantiatePrefab(Engine &engine, ID prefabId, float3 atPosition)
{
	const Prefab &prefab = GetPrefab(prefabId);

	ID entityIds[MAX_PREFAB_ENTITIES] = {};
	ID firstEntityId = {};
	for (u32 i = 0; i < prefab.entityCount; ++i)
	{
		EntityDesc entityDesc = prefab.entities[i];
		entityDesc.id = {}; // Each instance is a new entity, not the template's own
		entityDesc.pos = entityDesc.pos + atPosition;

		entityIds[i] = CreateEntity(engine, entityDesc);
		if (i == 0) {
			firstEntityId = entityIds[i];
		}
	}

	for (u32 i = 0; i < prefab.componentCount; ++i)
	{
		const ComponentDesc &component = prefab.components[i];
		if ( component.entityIndex < prefab.entityCount && entityIds[component.entityIndex] ) {
			AddComponent(engine, entityIds[component.entityIndex], component);
		}
	}

	return firstEntityId;
}


////////////////////////////////////////////////////////////////////////
// Tile grid management

float2 GetWorld2DCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const Window &window = GetWindow();
	const uint2 windowSize = { window.width, window.height };
	const float2 uvCoords = {(f32)pixelCoord.x/windowSize.x, 1.0f - (f32)pixelCoord.y/windowSize.y};
	const float2 ndcCoords = 2.0f * uvCoords - float2{1.0f, 1.0f};
	const f32 aspect = (f32)windowSize.x / (f32)windowSize.y;
	const float2 scale = {camera.height * aspect, camera.height};
	const float2 worldCoords = scale * ndcCoords + float2{camera.position.x, camera.position.y};
	return worldCoords;
}

int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const float2 worldCoords = GetWorld2DCoord(engine, camera, pixelCoord);
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	const int2 res = {(i32)Floor(worldCoords.x / cellWorldSize), (i32)Floor(worldCoords.y / cellWorldSize)};
	//LOG(Debug, "Tile coord: (%f, %f)\n", uvCoords.x, uvCoords.y);
	//LOG(Debug, "Tile coord: (%d, %d)\n", res.x, res.y);
	return res;
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, u32 collider, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x && coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].collider = collider;
	}
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, ID spriteId, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x && coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].spriteId = spriteId;
	}
}

static int2 WorldPosToGridCoord(float2 worldPos)
{
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	return int2{ (i32)Floor(worldPos.x / cellWorldSize), (i32)Floor(worldPos.y / cellWorldSize) };
}

static u32 GetColliderAtGridCoord(Scene &scene, int2 coord)
{
	for (u32 roomIndex = 0; roomIndex < scene.roomCount; ++roomIndex)
	{
		const Room &room = scene.rooms[roomIndex];
		const int2 localCoord = coord - room.pos;

		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			const Layer &layer = room.layers[i];
			if (!layer.initialized || !layer.isCollider) continue;

			if ( localCoord.x >= 0 && localCoord.x < layer.size.x &&
				localCoord.y >= 0 && localCoord.y < layer.size.y )
			{
				if (layer.cells[localCoord.x][localCoord.y].collider != 0)
				{
					return layer.cells[localCoord.x][localCoord.y].collider;
				}
			}
		}
	}
	return 0;
}

u32 GetColliderAtWorldPos(float2 worldPos)
{
	Engine &engine = GetEngine();
	return GetColliderAtGridCoord(engine.scene, WorldPosToGridCoord(worldPos));
}

// pos is the box's bottom-left corner, size its width/height (same convention as DrawBox).
// Tiles the box only touches at an edge count as colliding.
bool IsColliderInBox(float2 pos, float2 size, u32 collider)
{
	Engine &engine = GetEngine();
	const int2 minCoord = WorldPosToGridCoord(pos);
	const int2 maxCoord = WorldPosToGridCoord(pos + size);

	for (i32 y = minCoord.y; y <= maxCoord.y; ++y)
	{
		for (i32 x = minCoord.x; x <= maxCoord.x; ++x)
		{
			if (GetColliderAtGridCoord(engine.scene, int2{x, y}) == collider)
				return true;
		}
	}
	return false;
}



Layer &GetLayer(ID id)
{
	ASSERT( Valid(id) );
	Layer &layer = *((Layer*)GetObject(id));
	return layer;
}

u32 CreateLayer(Room &room, const LayerDesc &desc)
{
	u32 index = U32_MAX;

	if ( room.layerCount < ARRAY_COUNT(room.layers) )
	{
		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			Layer &layer = room.layers[i];
			if (!layer.initialized)
			{
				room.layerCount++;
				layer.initialized = true;
				layer.id = desc.id;
				layer.name = desc.name;
				layer.isBase = desc.isBase;
				layer.visible = desc.visible;
				layer.isCollider = desc.isCollider;
				layer.size = desc.size;
				index = i;

				BindID(&layer.id, &layer, IDKind_Layer);
				break;
			}
		}
	}

	return index;
}

void RemoveLayer(Room &room, u32 index)
{
	if ( index < ARRAY_COUNT(room.layers) )
	{
		Layer &layer = room.layers[index];
		// The base layer sets the room size and the parallax reference, so it always stays.
		if (layer.initialized && !layer.isBase)
		{
			Invalidate(layer.id);
			layer = {};
			room.layerCount--;
		}
	}
}

u32 MoveLayer(Room &room, u32 index, i32 delta)
{
	if ( delta == 0 || index >= ARRAY_COUNT(room.layers) ) return index;
	if ( !room.layers[index].initialized ) return index;

	const i32 slotCount = (i32)ARRAY_COUNT(room.layers);
	for (i32 i = (i32)index + delta; i >= 0 && i < slotCount; i += delta)
	{
		Layer &neighbour = room.layers[i];
		if (!neighbour.initialized) continue;

		const Layer moved = room.layers[index];
		room.layers[index] = neighbour;
		neighbour = moved;

		index = (u32)i;
		break;
	}

	for (i32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
	{
		Layer &layer = room.layers[i];
		if (layer.initialized)
		{
			SetObject(layer.id, &layer);
		}
	}

	return index; // Already at the end it was asked to move towards
}

const Layer *GetBaseLayer(const Room &room)
{
	for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
	{
		const Layer &layer = room.layers[i];
		if (layer.initialized && layer.isBase) return &layer;
	}
	return nullptr;
}

float2 LayerSize(const Layer &layer)
{
	const float2 res = Float2(layer.size);
	return res;
}

float2 RoomSize(const Room &room)
{
	const Layer *baseLayer = GetBaseLayer(room);
	const float2 res = baseLayer ? LayerSize(*baseLayer) : float2{0.0f, 0.0f};
	return res;
}

// Appends a room and gives it its ID. Null when the array is full. Rooms do not keep
// their descriptor around, so the ID comes in on its own.
static Room *PushRoom(Scene &scene, ID id)
{
	if ( scene.roomCount == MAX_ROOMS )
	{
		LOG(Warning, "Could not create room, the room array is full.\n");
		return nullptr;
	}

	Room &room = scene.rooms[scene.roomCount++];
	room = { .id = id };

	BindID(&room.id, &room, IDKind_Room);

	return &room;
}

// An empty room, with the two layers every room starts with
ID CreateRoom(Engine &engine)
{
	const RoomDesc desc = {
		.name = "Room",
		.layers = {
			{ .name = "Layer", .isBase = true, .visible = true, .size = {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} },
			{ .name = "Colliders", .visible = true, .isCollider = true, .size = {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} },
		},
		.layerCount = 2,
	};
	return CreateRoom(engine, desc);
}

ID CreateRoom(Engine &engine, const RoomDesc &desc)
{
	Room *roomPtr = PushRoom(engine.scene, desc.id);
	if ( !roomPtr ) {
		return {};
	}

	Room &room = *roomPtr;
	room.name = InternString(desc.name);
	room.pos = desc.pos;

	for (u32 l = 0; l < desc.layerCount; ++l)
	{
		LayerDesc layerDesc = desc.layers[l];
		layerDesc.name = InternString(layerDesc.name);

		const u32 index = CreateLayer(room, layerDesc);
		if (index == U32_MAX) {
			continue;
		}

		Layer &layer = room.layers[index];
		if (layerDesc.isCollider) {
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					layer.cells[tile.x][tile.y].collider = tile.collider;
				}
			}
		}
		else
		{
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					if ( tile.spriteId.slot != 0 && !Valid(tile.spriteId) ) {
						LOG(Warning, "Layer <%s> has a tile at (%u, %u) referring to sprite ID %u, which does not exist.\n",
								layerDesc.name, tile.x, tile.y, tile.spriteId.slot);
						continue;
					}
					layer.cells[tile.x][tile.y].spriteId = tile.spriteId;
				}
			}
		}
	}

	return room.id;
}

ID CreateRoom(Engine &engine, const BinRoom &binRoom)
{
	const BinRoomDesc &bin = *binRoom.desc;

	RoomDesc desc = {};
	desc.id = bin.id;
	desc.name = bin.name;
	desc.pos = bin.pos;

	for (u32 l = 0; l < bin.layerCount && l < ARRAY_COUNT(desc.layers); ++l)
	{
		const BinLayerDesc &ld = bin.layers[l];
		desc.layers[desc.layerCount++] = {
			.name = ld.name,
			.isBase = ld.isBase != 0,
			.visible = ld.visible != 0,
			.isCollider = ld.isCollider != 0,
			.size = ld.size,
			.tiles = binRoom.tiles[l],
			.tileCount = ld.tiles.size / (u32)sizeof(TileDesc),
		};
	}

	return CreateRoom(engine, desc);
}

void RemoveRoom(Engine &engine, ID id)
{
	// Marks only, see RemoveEntity
	if (id)
	{
		Room &room = GetRoom(id);

		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			if (room.layers[i].initialized) {
				Invalidate(room.layers[i].id);
			}
		}

		room.id = {};
		Invalidate(id);
	}
}

void CreateScene(Engine &engine)
{
	engine.scene.ambientLight = Float3(1.0f);
	CreateRoom(engine);
}

void CleanScene(Engine &engine)
{
	GameStop(engine);

	WaitDeviceIdle(engine.gfx.device);

	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Audio &audio = engine.audio;

	AudioStopAll(audio);
	ClearParticles(scene);

	if (PopDataArenaState(engine))
	{
	}

	// Mark everything the scene owns
	for (u16 i = 0; i < gfx.textureCount; ++i) {
		if ( !(gfx.textures[i].desc.flags & AssetFlag_Builtin) ) {
			RemoveTexture(gfx, gfx.textures[i].desc.id);
		}
	}
	for (u16 i = 0; i < gfx.materialCount; ++i) {
		if ( !(gfx.materials[i].desc.flags & AssetFlag_Builtin) ) {
			RemoveMaterial(gfx, gfx.materials[i].desc.id);
		}
	}
	for (u16 i = 0; i < scene.roomCount; ++i) {
		RemoveRoom(engine, scene.rooms[i].id);
	}
	for (u16 i = 0; i < scene.entityCount; ++i) {
		RemoveEntity(engine, scene.entities[i].id);
	}
	for (u16 i = 0; i < scene.spriteCount; ++i) {
		RemoveSprite(scene, scene.sprites[i].desc.id);
	}
	for (u16 i = 0; i < scene.particleEffectCount; ++i) {
		if ( !IsBuiltin(scene.particleEffects[i].desc.id) ) {
			RemoveParticleEffect(scene, scene.particleEffects[i].desc.id);
		}
	}
	for (u16 i = 0; i < scene.prefabCount; ++i) {
		RemovePrefab(scene, scene.prefabs[i].id);
	}
	for (u16 i = 0; i < audio.clipCount; ++i) {
		RemoveAudioClip(audio.clips[i].desc.id);
	}
	for (u32 i = 0; i < audio.musicFileCount; ++i) {
		DestroyMusicFile(audio.musicFiles[i].desc.id);
	}

	// Compaction after removal
	CompactRooms(engine.scene);
	CompactEntities(engine.scene);
	CompactSprites(engine.scene);
	CompactParticleEffects(engine.scene);
	CompactPrefabs(engine.scene);
	CompactMaterials(engine.gfx);
	CompactTextures(engine.gfx);
	// The audio pools are not compacted here: only the mixing thread may move that

	CloseAssets(engine.assets);

	engine.gfx.shouldUpdateMaterials = true;
	engine.gfx.shouldUpdateMaterialBindGroups = true;
}
