#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "lara.h"
#include "enemy.h"
#include "camera.h"
#include "trigger.h"

#ifdef _DEBUG
    #include "debug.h"
#endif

const char SHADER[] =
    #include "shader.glsl"
;

struct Level {
    enum { shStatic, shCaustics, shSprite, shMAX };

    TR::Level   level;
    Shader      *shaders[shMAX];
    Texture     *atlas;
    MeshBuilder *mesh;

    Lara        *lara;
    Camera      *camera;

    float       time;

    Level(Stream &stream, bool demo) : level{stream, demo}, time(0.0f), lara(NULL) {
        #ifdef _DEBUG
            Debug::init();
        #endif
        mesh = new MeshBuilder(level);
        
        initAtlas();
        initShaders();
        initOverrides();

        for (int i = 0; i < level.entitiesCount; i++) {
            TR::Entity &entity = level.entities[i];
            switch (entity.type) {
                case TR::Entity::LARA : 
                case TR::Entity::LARA_CUT :
                    entity.controller = (lara = new Lara(&level, i));
                    break;
                case TR::Entity::ENEMY_WOLF            :   
                    entity.controller = new Wolf(&level, i);
                    break;
                case TR::Entity::ENEMY_BEAR            : 
                    entity.controller = new Bear(&level, i);
                    break;
                case TR::Entity::ENEMY_BAT             :   
                    entity.controller = new Bat(&level, i);
                    break;
                case TR::Entity::ENEMY_TWIN            :   
                case TR::Entity::ENEMY_CROCODILE_LAND  :   
                case TR::Entity::ENEMY_CROCODILE_WATER :   
                case TR::Entity::ENEMY_LION_MALE       :   
                case TR::Entity::ENEMY_LION_FEMALE     :   
                case TR::Entity::ENEMY_PUMA            :   
                case TR::Entity::ENEMY_GORILLA         :   
                case TR::Entity::ENEMY_RAT_LAND        :   
                case TR::Entity::ENEMY_RAT_WATER       :   
                case TR::Entity::ENEMY_REX             :   
                case TR::Entity::ENEMY_RAPTOR          :   
                case TR::Entity::ENEMY_MUTANT          :   
                case TR::Entity::ENEMY_CENTAUR         :   
                case TR::Entity::ENEMY_MUMMY           :   
                case TR::Entity::ENEMY_LARSON          :
                    entity.controller = new Enemy(&level, i);
                    break;
                case TR::Entity::DOOR_1                :
                case TR::Entity::DOOR_2                :
                case TR::Entity::DOOR_3                :
                case TR::Entity::DOOR_4                :
                case TR::Entity::DOOR_5                :
                case TR::Entity::DOOR_6                :
                case TR::Entity::DOOR_BIG_1            :
                case TR::Entity::DOOR_BIG_2            :
                case TR::Entity::DOOR_FLOOR_1          :
                case TR::Entity::DOOR_FLOOR_2          :
                case TR::Entity::TRAP_FLOOR            :
                case TR::Entity::TRAP_BLADE            :
                case TR::Entity::TRAP_SPIKES           :
                    entity.controller = new Trigger(&level, i, true);
                    break;
                case TR::Entity::TRAP_BOULDER          :
                    entity.controller = new Boulder(&level, i);
                    break;
                case TR::Entity::TRAP_DARTGUN          :
                    entity.controller = new Dartgun(&level, i);
                    break;
                case TR::Entity::BLOCK_1 :
                case TR::Entity::BLOCK_2 :
                    entity.controller = new Block(&level, i);
                    break;
                case TR::Entity::SWITCH                :
                case TR::Entity::SWITCH_WATER          :
                case TR::Entity::HOLE_PUZZLE           :
                case TR::Entity::HOLE_KEY              :
                    entity.controller = new Trigger(&level, i, false);
                    break;
                default : ;
            }
        }

        ASSERT(lara != NULL);
        camera = new Camera(&level, lara);

        level.cameraController = camera;
    }

    ~Level() {
        #ifdef _DEBUG
            Debug::free();
        #endif
        for (int i = 0; i < level.entitiesCount; i++)
            delete (Controller*)level.entities[i].controller;

        for (int i = 0; i < shMAX; i++)
            delete shaders[i];

        delete atlas;
        delete mesh;

        delete camera;        
    }

    void initAtlas() {
        if (!level.tilesCount) {
            atlas = NULL;
            return;
        }

        TR::RGBA *data = new TR::RGBA[1024 * 1024];
        for (int i = 0; i < level.tilesCount; i++) {
            int tx = (i % 4) * 256;
            int ty = (i / 4) * 256;

            TR::RGBA *ptr = &data[ty * 1024 + tx];
            for (int y = 0; y < 256; y++) {
                for (int x = 0; x < 256; x++) {
                    int index = level.tiles[i].index[y * 256 + x];
                    auto p = level.palette[index];
                    ptr[x].r = p.r;
                    ptr[x].g = p.g;
                    ptr[x].b = p.b;
                    ptr[x].a = index == 0 ? 0 : 255;
                }
                ptr += 1024;
            }
        }

        for (int y = 1020; y < 1024; y++)
            for (int x = 1020; x < 1024; x++) {
                int i = y * 1024 + x;
                data[i].r = data[i].g = data[i].b = data[i].a = 255;    // white texel for colored triangles
            }

        atlas = new Texture(1024, 1024, 0, data);
        delete[] data;
        PROFILE_LABEL(TEXTURE, atlas->ID, "atlas");
    }

    void initShaders() {
        char def[255], ext[255];
        sprintf(def, "#define MAX_RANGES %d\n#define MAX_OFFSETS %d\n", mesh->animTexRangesCount, mesh->animTexOffsetsCount);
        shaders[shStatic]   = new Shader(SHADER, def);
        sprintf(ext, "%s#define CAUSTICS\n", def);
        shaders[shCaustics] = new Shader(SHADER, ext);
        sprintf(ext, "%s#define SPRITE\n", def);
        shaders[shSprite]   = new Shader(SHADER, ext);
    }

    void initOverrides() {
    /*
        for (int i = 0; i < level.entitiesCount; i++) {
            int16 &id = level.entities[i].id;
            switch (id) {
            // weapon
                case 84 : id =  99; break; // pistols
                case 85 : id = 100; break; // shotgun
                case 86 : id = 101; break; // magnums
                case 87 : id = 102; break; // uzis
            // ammo
                case 88 : id = 103; break; // for pistols
                case 89 : id = 104; break; // for shotgun
                case 90 : id = 105; break; // for magnums
                case 91 : id = 106; break; // for uzis
            // medikit
                case 93 : id = 108; break; // big
                case 94 : id = 109; break; // small
            // keys
                case 110 : id = 114; break; 
                case 111 : id = 115; break; 
                case 112 : id = 116; break; 
                case 113 : id = 117; break; 
                case 126 : id = 127; break; 
                case 129 : id = 133; break; 
                case 130 : id = 134; break; 
                case 131 : id = 135; break; 
                case 132 : id = 136; break; 
                case 141 : id = 145; break; 
                case 142 : id = 146; break; 
                case 143 : id = 150; break; 
                case 144 : id = 150; break;
            }
        }
    */
    }

    Shader *setRoomShader(const TR::Room &room, float intensity) {
        if (room.flags.water) {
            Core::color = vec4(0.6f * intensity, 0.9f * intensity, 0.9f * intensity, 1.0f);
            return shaders[shCaustics];
        } else {
            Core::color = vec4(intensity, intensity, intensity, 1.0f);
            return shaders[shStatic];
        }
    }

    void renderRoom(int roomIndex, int from = -1) {
        ASSERT(roomIndex >= 0 && roomIndex < level.roomsCount);
        PROFILE_MARKER("ROOM");

        TR::Room &room = level.rooms[roomIndex];
        vec3 offset = vec3(room.info.x, 0.0f, room.info.z);

        Shader *sh = setRoomShader(room, 1.0f);

        sh->bind();
        sh->setParam(uColor, Core::color);

    // room static meshes
        {
            PROFILE_MARKER("R_MESH");

            for (int i = 0; i < room.meshesCount; i++) {
                TR::Room::Mesh &rMesh = room.meshes[i];
                if (rMesh.flags.rendered) continue;    // skip if already rendered

                TR::StaticMesh *sMesh = level.getMeshByID(rMesh.meshID);
                ASSERT(sMesh != NULL);

            // check visibility
                Box box;
                vec3 offset = vec3(rMesh.x, rMesh.y, rMesh.z);
                sMesh->getBox(false, rMesh.rotation, box);
                if (!camera->frustum->isVisible(offset + box.min, offset + box.max))
                    continue;
                rMesh.flags.rendered = true;

            // set light parameters
                getLight(offset, roomIndex);

            // render static mesh
                mat4 mTemp = Core::mModel;
                Core::mModel.translate(offset);
                Core::mModel.rotateY(rMesh.rotation);
                renderMesh(sMesh->mesh);
                Core::mModel = mTemp;
            }
        }

    // room geometry & sprites
        if (!room.flags.rendered) {    // skip if already rendered

            mat4 mTemp = Core::mModel;
            {
                PROFILE_MARKER("R_GEOM");

                room.flags.rendered = true;

                Core::lightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                Core::ambient    = vec3(1.0f);
                sh->setParam(uLightColor, Core::lightColor);
                sh->setParam(uAmbient, Core::ambient);

                Core::mModel.translate(offset);

            // render room geometry
                sh->setParam(uModel, Core::mModel);
                mesh->renderRoomGeometry(roomIndex);
            }

        // render room sprites
            if (mesh->hasRoomSprites(roomIndex)) {
                PROFILE_MARKER("R_SPR");
                sh = shaders[shSprite];
                sh->bind();
                sh->setParam(uModel, Core::mModel);
                sh->setParam(uColor, Core::color);
                mesh->renderRoomSprites(roomIndex);
            }

            Core::mModel = mTemp;
        }

    // render rooms through portals recursively
        Frustum *camFrustum = camera->frustum;   // push camera frustum
        Frustum frustum;
        camera->frustum = &frustum;

        for (int i = 0; i < room.portalsCount; i++) {
            TR::Room::Portal &p = room.portals[i];

            if (p.roomIndex == from) continue;

            vec3 v[] = {
                offset + p.vertices[0],
                offset + p.vertices[1],
                offset + p.vertices[2],
                offset + p.vertices[3],
            };

            frustum = *camFrustum;
            if (frustum.clipByPortal(v, 4, p.normal))
                renderRoom(p.roomIndex, roomIndex);
        }
        camera->frustum = camFrustum;    // pop camera frustum
    }

    void renderMesh(uint32 offsetIndex) {
        MeshBuilder::MeshInfo *m = mesh->meshMap[offsetIndex];
        if (!m) return; // invisible mesh (offsetIndex > 0 && level.meshOffsets[offsetIndex] == 0) camera target entity etc.

        Core::active.shader->setParam(uModel, Core::mModel);
        mesh->renderMesh(m);
    }

    void renderShadow(const vec3 &pos, const vec3 &offset, const vec3 &size, float angle) {
        mat4 m;
        m.identity();
        m.translate(pos);
        m.rotateY(angle);
        m.translate(vec3(offset.x, 0.0f, offset.z));
        m.scale(vec3(size.x, 0.0f, size.z) * (1.0f / 1024.0f));

        Core::active.shader->setParam(uModel, m);
        Core::active.shader->setParam(uColor, vec4(0.0f, 0.0f, 0.0f, 0.5f));
        mesh->renderShadowSpot();
    }

    void renderModel(const TR::Model &model, const TR::Entity &entity) {
        TR::Animation *anim;
        float fTime;
        vec3 angle;

        Controller *controller = (Controller*)entity.controller;

        if (controller) {
            anim  = &level.anims[controller->animIndex];
            angle = controller->angle;
            fTime = controller->animTime;
        } else {
            anim  = &level.anims[model.animation];
            angle = vec3(0.0f, entity.rotation, 0.0f);
            fTime = time;
        }

        if (angle.y != 0.0f) Core::mModel.rotateY(angle.y);
        if (angle.x != 0.0f) Core::mModel.rotateX(angle.x);
        if (angle.z != 0.0f) Core::mModel.rotateZ(angle.z);

        float k = fTime * 30.0f / anim->frameRate;
        int fIndex = (int)k;
        int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

        int fSize = sizeof(TR::AnimFrame) + model.mCount * sizeof(uint16) * 2;
        k = k - fIndex;

        int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
        TR::AnimFrame *frameA = (TR::AnimFrame*)&level.frameData[(anim->frameOffset + fIndexA * fSize) >> 1];

        TR::Animation *nextAnim = NULL;

        vec3 move(0.0f);
        if (fIndexB == 0) {
            if (controller)
                move = controller->getAnimMove();
            nextAnim = &level.anims[anim->nextAnimation];
            fIndexB = (anim->nextFrame - nextAnim->frameStart) / nextAnim->frameRate;
        } else
            nextAnim = anim;
        
        TR::AnimFrame *frameB = (TR::AnimFrame*)&level.frameData[(nextAnim->frameOffset + fIndexB * fSize) >> 1];

        vec3 bmin = frameA->box.min().lerp(frameB->box.min(), k);
        vec3 bmax = frameA->box.max().lerp(frameB->box.max(), k);
        if (!camera->frustum->isVisible(Core::mModel, bmin, bmax))
            return;

        TR::Node *node = (int)model.node < level.nodesDataSize ? (TR::Node*)&level.nodesData[model.node] : NULL;

        mat4 m;
        m.identity();
        m.translate(((vec3)frameA->pos).lerp(move + frameB->pos, k));

        int sIndex = 0;
        mat4 stack[20];

        for (int i = 0; i < model.mCount; i++) {

            if (i > 0 && node) {
                TR::Node &t = node[i - 1];

                if (t.flags & 0x01) m = stack[--sIndex];
                if (t.flags & 0x02) stack[sIndex++] = m;

                ASSERT(sIndex >= 0 && sIndex < 20);

                m.translate(vec3(t.x, t.y, t.z));
            }

            quat q;
            if (entity.type == TR::Entity::LARA && (((Lara*)controller)->animOverrideMask & (1 << i)))
                q = ((Lara*)controller)->animOverrides[i];
            else
                q = lerpAngle(frameA->getAngle(i), frameB->getAngle(i), k);
            m = m * mat4(q, vec3(0.0f));


        //  vec3 angle = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
        //  m.rotateY(angle.y);
        //  m.rotateX(angle.x);
        //  m.rotateZ(angle.z);

            mat4 tmp = Core::mModel;
            Core::mModel = Core::mModel * m;
            if (controller)
                renderMesh(controller->meshes[i]);
            else
                renderMesh(model.mStart + i);
            Core::mModel = tmp;
        }

        if (TR::castShadow(entity.type)) {
            TR::Level::FloorInfo info;
            level.getFloorInfo(entity.room, entity.x, entity.z, info, true);
            renderShadow(vec3(entity.x, info.floor - 16.0f, entity.z), (bmax + bmin) * 0.5f, (bmax - bmin) * 0.8f, entity.rotation);
        }
    }

    void renderSequence(const TR::Entity &entity) {
        shaders[shSprite]->bind();
        Core::active.shader->setParam(uModel, Core::mModel);
        Core::active.shader->setParam(uColor, Core::color);

        int sIndex = -(entity.modelIndex + 1);        
        int sFrame;
        if (entity.controller)
            sFrame = ((SpriteController*)entity.controller)->frame;
        else
            sFrame = int(time * 10.0f) % level.spriteSequences[sIndex].sCount;
        mesh->renderSprite(sIndex, sFrame);
    }

    int getLightIndex(const vec3 &pos, int &room) {
        int idx = -1;
        float dist;
      //  for (int j = 0; j < level.roomsCount; j++)
        int j = room;
            for (int i = 0; i < level.rooms[j].lightsCount; i++) {
                TR::Room::Light &light = level.rooms[j].lights[i];
                float d = (pos - vec3(light.x, light.y, light.z)).length2();
                if (idx == -1 || d < dist) {
                    idx = i;
                    dist = d;
                //    room = j;
                }
            }
        return idx;
    }

    void getLight(const vec3 &pos, int roomIndex) {
        int room = roomIndex;
        int idx = getLightIndex(pos, room);

        if (idx > -1) {
            TR::Room::Light &light = level.rooms[room].lights[idx];
            float c = level.rooms[room].lights[idx].intensity / 8191.0f;
            Core::lightPos   = vec3(light.x, light.y, light.z);
            Core::lightColor = vec4(c, c, c, (float)light.attenuation * (float)light.attenuation);
        } else {
            Core::lightPos   = vec3(0);
            Core::lightColor = vec4(0, 0, 0, 1);
        }

        Core::ambient = vec3(1.0f - level.rooms[roomIndex].ambient / 8191.0f);
        Core::active.shader->setParam(uAmbient, Core::ambient);
        Core::active.shader->setParam(uLightPos, Core::lightPos);
        Core::active.shader->setParam(uLightColor, Core::lightColor);
    }

    void renderEntity(const TR::Entity &entity) {
        if (entity.type == TR::Entity::NONE) return;

        TR::Room &room = level.rooms[entity.room];
        if (!room.flags.rendered || entity.flags.invisible) // check for room visibility
            return;

        mat4 m = Core::mModel;
        Core::mModel.translate(vec3(entity.x, entity.y, entity.z));

        float c = (entity.intensity > -1) ? (1.0f - entity.intensity / (float)0x1FFF) : 1.0f;
        float l = 1.0f;

    // set shader
        setRoomShader(room, c)->bind();
        Core::active.shader->setParam(uColor, Core::color);

    // get light parameters for entity
        getLight(vec3(entity.x, entity.y, entity.z), entity.room);

    // render entity models
        if (entity.modelIndex > 0) {
            PROFILE_MARKER("MDL");
            renderModel(level.models[entity.modelIndex - 1], entity);
        }
    // if entity is billboard
        if (entity.modelIndex < 0) {
            PROFILE_MARKER("SPR");
            Core::color = vec4(c, c, c, 1.0f);
            renderSequence(entity);
        }

        Core::mModel = m;
    }

    void update() {
        time += Core::deltaTime;
        
        for (int i = 0; i < level.entitiesCount; i++) 
            if (level.entities[i].type != TR::Entity::NONE) {
                Controller *controller = (Controller*)level.entities[i].controller;
                if (controller) 
                    controller->update();
            }
        
        camera->update();
    }

    void setup() {
        PROFILE_MARKER("SETUP");

        camera->setup();;
        atlas->bind(0);

        if (!Core::support.VAO)
            mesh->bind();

        // set frame constants for all shaders
        Core::active.shader = NULL;
        for (int i = 0; i < shMAX; i++) {
            shaders[i]->bind();
            shaders[i]->setParam(uViewProj, Core::mViewProj);
            shaders[i]->setParam(uViewInv, Core::mViewInv);
            shaders[i]->setParam(uViewPos, Core::viewPos);
            shaders[i]->setParam(uParam, vec4(time, 0, 0, 0));
            shaders[i]->setParam(uAnimTexRanges, mesh->animTexRanges[0], mesh->animTexRangesCount);
            shaders[i]->setParam(uAnimTexOffsets, mesh->animTexOffsets[0], mesh->animTexOffsetsCount);
        }
        glEnable(GL_DEPTH_TEST);

        Core::setCulling(cfFront);

        Core::mModel.identity();

        // clear visible flags for rooms & static meshes
        for (int i = 0; i < level.roomsCount; i++) {
            TR::Room &room = level.rooms[i];
            room.flags.rendered = false;                            // clear visible flag for room geometry & sprites

            for (int j = 0; j < room.meshesCount; j++)
                room.meshes[j].flags.rendered = false;     // clear visible flag for room static meshes
        }    
    }

    void renderRooms() {
        PROFILE_MARKER("ROOMS");
        renderRoom(camera->getRoomIndex());
    }

    void renderEntities() {
        PROFILE_MARKER("ENTITIES");

        shaders[shStatic]->bind();
        for (int i = 0; i < level.entitiesCount; i++)
            renderEntity(level.entities[i]);
    }

    void renderScene() {
        PROFILE_MARKER("SCENE");
        setup();
        renderRooms();
        renderEntities();
    }

    void render() {
        renderScene();
    #ifdef _DEBUG
        Debug::begin();
        //    Debug::Level::rooms(level, lara->pos, lara->getEntity().room);
        //    Debug::Level::lights(level);
        //    Debug::Level::portals(level);
        //    Debug::Level::meshes(level);
        //    Debug::Level::entities(level);
        Debug::Level::info(level, lara->getEntity(), (int)lara->state, lara->animIndex, int(lara->animTime * 30.0f));
        Debug::end();
    #endif
    }
};

#endif