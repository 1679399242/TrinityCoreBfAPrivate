#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "AreaTriggerTemplate.h"
#include "AreaTriggerAI.h"
#include "AreaTrigger.h"
#include "Vehicle.h"
#include "new_karazhan.h"

enum Spells
{
    // Attumen
    SPELL_SPAWN_MIDNIGHT                = 227298,
    SPELL_MORTAL_STRIKE                 = 227493,
    SPELL_MOUNTED_STRIKE                = 227636,
    SPELL_STAMP                         = 227638,
    SPELL_SHARED_SUFFERING              = 228852,
    SPELL_MOUNTED                       = 227584,
    SPELL_FORCE_RIDE_VEHICLE            = 46598,

    // Midnight
    SPELL_MEZAIR                        = 227339,
    SPELL_MIGHTY_STOMP                  = 227363,
    SPELL_SPECTRAL_CHARGE               = 227365,
    SPELL_SPECTRAL_CHARGE_AREA          = 227367,
    SPELL_SPECTRAL_CHARGE_DMG           = 227645,
    SPELL_INTANGIBLE_PRESENCE           = 227404,
    SPELL_INTANGIBLE_PRESENCE_DISPEL    = 227419,
    SPELL_INTANGIBLE_PRESENCE_DMG       = 227415,
    SPELL_INTANGIBLE_PRESENCE_VISUAL    = 227436,
    SPELL_ENRAGE                        = 228895,
    SPELL_RIDERLESS                     = 227338,

    // Intangible Pressence
    SPELL_GHOST_VISUAL                  = 188272,
    SPELL_GHOST_VISUAL_2                = 221829,
    SPELL_GHOSTLY_COSMETIC              = 65202,
};

enum Events
{
    // Attumen the horseman
    EVENT_MORTAL_STRIKE         = 1,
    EVENT_SHARED_SUFFERING      = 2,
    EVENT_INTAGIBLE_PRESSENCE   = 3,
    EVENT_MOUNTED_STRIKE        = 4,

    // Midnight
    EVENT_SPECTRAL_CHARGE       = 5,
    EVENT_MIGHTY_STOMP          = 6,
    EVENT_MEZAIR                = 7,
    EVENT_STAMP                 = 8,
    EVENT_FOLLOW_TARGET         = 9,
    EVENT_MOUNT_VEHICLE         = 10,
};

enum Actions
{
    ACTION_MOUNT_MIDNIGHT       = 1,
    ACTION_DISMOUNT_MIDNIGHT    = 2,
    ACTION_END_ENCOUNTER        = 3,
    ACTION_ATTUMEN_DIED         = 4,
    ACTION_FULL_HEALTH          = 5,
    ACTION_MIDNIGHT_BOARDED     = 6,
};

enum Points
{
    POINT_SPECTRAL_CHARGE   = 1,
    POINT_MEZAIR            = 2,
    POINT_HOME              = 3,
};

enum Adds
{
    NPC_MIDNIGHT            = 114264,
    NPC_INTAGIBLE_PRESENCE  = 114315,
};

enum Says
{
    SAY_INTRO               = 0,
    SAY_AGGRO               = 1,
    SAY_INTAGIBLE_PRESSENCE = 2,
    SAY_MIGHTY_STOMP        = 3,
    SAY_FOOT_PHASE          = 4,
    SAY_RIDE_PHASE          = 5,
    SAY_SPECTRAL_CHARGE     = 6,
    SAY_KILL                = 7,
    SAY_KILL_MIDNIGHT       = 8,
    SAY_WIPE                = 9,
    SAY_DEATH               = 10,
};

constexpr uint32 PHASE_ONE      = 1;
constexpr uint32 PHASE_TWO      = 2;
constexpr int32 DATA_PASSENGER  = 1;

Position CenterOfRoom;

using G3D::Vector3;
using G3D::Vector2;

class boss_attumen_new : public CreatureScript
{
    public:
        boss_attumen_new() : CreatureScript("boss_attumen_new")
        {}

        struct boss_attumen_AI : public BossAI
        {
            explicit boss_attumen_AI(Creature* creature) : BossAI(creature, DATA_ATTUMEN)
            {
                CenterOfRoom = me->GetHomePosition();
            }

            void Reset() override
            {
                _Reset();
            }

            void SummonHorse()
            {
                me->SummonCreature(NPC_MIDNIGHT, me->GetPosition(), TEMPSUMMON_CORPSE_DESPAWN, 5000);
            }

            void InitializeAI() override
            {
                if (!me->isDead())
                    Reset();

                SummonHorse();
            }

            void EnterCombat(Unit* /**/) override
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                Talk(SAY_AGGRO);
                _EnterCombat();

                events.ScheduleEvent(EVENT_MIGHTY_STOMP, Seconds(17), PHASE_ONE);
                events.ScheduleEvent(EVENT_MOUNTED_STRIKE, Seconds(urand(8, 12)), PHASE_ONE);
                events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, Seconds(10), PHASE_ONE);
                events.ScheduleEvent(EVENT_STAMP, Seconds(urand(5, 8)), PHASE_ONE);
            }

            void JustDied(Unit* /**/) override
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (Creature* midnight = me->FindNearestCreature(NPC_MIDNIGHT, 500.f))
                    midnight->GetAI()->DoAction(ACTION_ATTUMEN_DIED);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_MOUNT_MIDNIGHT:
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                        events.CancelEventGroup(PHASE_TWO);
                        me->GetMotionMaster()->MovePoint(POINT_HOME, me->GetHomePosition());
                        break;
                    }

                    case ACTION_DISMOUNT_MIDNIGHT:
                    {
                        Talk(SAY_FOOT_PHASE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                        events.CancelEventGroup(PHASE_ONE);
                        events.ScheduleEvent(EVENT_MORTAL_STRIKE, Seconds(10), PHASE_TWO);
                        events.ScheduleEvent(EVENT_SHARED_SUFFERING, Seconds(15), PHASE_TWO);
                        events.ScheduleEvent(EVENT_MEZAIR, Seconds(5), PHASE_TWO);
                        break;
                    }

                    case ACTION_MIDNIGHT_BOARDED:
                    {
                        events.ScheduleEvent(EVENT_SPECTRAL_CHARGE, Seconds(5), PHASE_ONE);
                        events.ScheduleEvent(EVENT_MIGHTY_STOMP, Seconds(17), PHASE_ONE);
                        events.ScheduleEvent(EVENT_MOUNTED_STRIKE, Seconds(urand(8, 12)), PHASE_ONE);
                        events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, Seconds(30), PHASE_ONE);
                        break;
                    }

                    case ACTION_END_ENCOUNTER:
                    {
                        _JustDied();
                        break;
                    }

                    default : break;
                }
            }

            void JustReachedHome() override
            {
                Talk(SAY_WIPE);
                _JustReachedHome();
                SummonHorse();
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim && victim->GetTypeId())
                    Talk(SAY_KILL);
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon && summon->GetEntry() == NPC_MIDNIGHT)
                    me->EnterVehicle(summon);

                BossAI::JustSummoned(summon);
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (Creature* midnight = me->FindNearestCreature(NPC_MIDNIGHT, 500.f))
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, midnight);
                    midnight->DespawnOrUnsummon();
                }

                me->RemoveAllAreaTriggers();
                CreatureAI::EnterEvadeMode(why);
            }

            void ExecuteEvent(uint32 eventId) override
            {
                switch (eventId)
                {
                    case EVENT_MORTAL_STRIKE:
                    {
                        DoCastVictim(SPELL_MORTAL_STRIKE);
                        break;
                    }

                    case EVENT_MOUNTED_STRIKE:
                    {
                        DoCastVictim(SPELL_MOUNTED_STRIKE);
                        events.ScheduleEvent(EVENT_MOUNTED_STRIKE, Seconds(urand(10, 13)), PHASE_ONE);
                        break;
                    }

                    case EVENT_SHARED_SUFFERING:
                    {
                        DoCastSelf(SPELL_SHARED_SUFFERING);
                        break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_attumen_AI(creature);
        }
};

class npc_kara_midnight : public CreatureScript
{
    public:
        npc_kara_midnight() : CreatureScript("npc_kara_midnight")
        {}

        struct npc_kara_midnight_AI : public ScriptedAI
        {
            explicit npc_kara_midnight_AI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
                _attumenDied = false;
                _attumenGUID = ObjectGuid::Empty;
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE)
                {
                    switch (id)
                    {
                        case POINT_HOME:
                        {
                            me->RemoveAurasDueToSpell(SPELL_RIDERLESS);
                            Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID);

                            if (attumen)
                            {
                                attumen->EnterVehicle(me);
                                attumen->GetAI()->DoAction(ACTION_MIDNIGHT_BOARDED);
                                attumen->AI()->Talk(SAY_RIDE_PHASE);
                            }

                            me->SetReactState(REACT_AGGRESSIVE);
                            _events.CancelEventGroup(PHASE_TWO);
                            _events.ScheduleEvent(EVENT_FOLLOW_TARGET, 500, PHASE_ONE);
                            _events.ScheduleEvent(EVENT_SPECTRAL_CHARGE, Seconds(10), PHASE_ONE);
                            _events.ScheduleEvent(EVENT_MIGHTY_STOMP, Seconds(30), PHASE_ONE);
                            _events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, Seconds(15), PHASE_ONE);
                            _events.ScheduleEvent(EVENT_STAMP, Seconds(urand(8, 12)), PHASE_ONE);
                            break;
                        }

                        case POINT_MEZAIR:
                        {
                            me->AttackStop();
                            me->StopMoving();
                            Creature* attumen = me->FindNearestCreature(BOSS_ATTUMEN, 500);
                            if (attumen)
                            {
                                me->SetFacingToObject(attumen, true);
                                DoCastSelf(SPELL_MEZAIR);
                            }
                            _events.ScheduleEvent(EVENT_MEZAIR, 5000, PHASE_TWO);
                            break;
                        }

                        case POINT_SPECTRAL_CHARGE:
                        {
                            DoCastSelf(SPELL_SPECTRAL_CHARGE);
                            if (_chargesDone++ < 3)
                                _events.ScheduleEvent(EVENT_SPECTRAL_CHARGE, Seconds(5), PHASE_ONE);
                            else
                            {
                                _events.ScheduleEvent(EVENT_FOLLOW_TARGET, 500, PHASE_ONE);
                                _events.ScheduleEvent(EVENT_SPECTRAL_CHARGE, Seconds(15), PHASE_ONE);
                            }
                            break;
                        }

                        default : break;
                    }
                }
            }

            void PassengerBoarded(Unit* passenger, int8 /**/, bool /**/) override
            {
                if (passenger && passenger->GetEntry() == BOSS_ATTUMEN)
                {
                    _attumenGUID = passenger->GetGUID();
                    passenger->CastSpell(passenger, SPELL_MOUNTED, true);
                }
            }

            void JustDied(Unit* /**/) override
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                auto* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID);

                _events.Reset();
                _summons.DespawnAll();
                me->RemoveAllAreaTriggers();

                if (attumen)
                    attumen->GetAI()->DoAction(ACTION_END_ENCOUNTER);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim && victim->GetTypeId() == TYPEID_PLAYER)
                {
                    Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID);

                    if (attumen)
                        attumen->AI()->Talk(SAY_KILL_MIDNIGHT);
                }
            }

            void JustSummoned(Creature* summon) override
            {
                _summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                _summons.Summon(summon);
            }

            void EnterCombat(Unit* /**/) override
            {
                DoZoneInCombat(me, 500.f);
                _chargesDone = 0;

                if (InstanceScript* instance = me->GetInstanceScript())
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                if (Creature* rider = ObjectAccessor::GetCreature(*me, _attumenGUID))
                    rider->AI()->EnterCombat(nullptr);

                _events.ScheduleEvent(EVENT_MIGHTY_STOMP, Seconds(17), PHASE_ONE);
                _events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, Seconds(10), PHASE_ONE);
                _events.ScheduleEvent(EVENT_STAMP, Seconds(urand(5, 8)), PHASE_ONE);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_FULL_HEALTH:
                    {
                        Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID);

                        if (attumen)
                        {
                            me->GetMotionMaster()->MovePoint(POINT_HOME, attumen->GetHomePosition());
                            attumen->GetAI()->DoAction(ACTION_MOUNT_MIDNIGHT);
                        }
                        break;
                    }

                    case ACTION_ATTUMEN_DIED:
                    {
                        _attumenDied = true;
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveAurasDueToSpell(SPELL_RIDERLESS);
                        _events.CancelEventGroup(PHASE_TWO);
                        _events.RescheduleEvent(EVENT_MIGHTY_STOMP, Seconds(15));
                        me->CastSpell(me, SPELL_ENRAGE, true);
                        break;
                    }
                }
            }

            void Reset() override
            {
                _events.Reset();
                _summons.DespawnAll();
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                if (Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID))
                    attumen->AI()->EnterEvadeMode(why);

                CreatureAI::EnterEvadeMode(why);
            }

            void DamageTaken(Unit* /**/, uint32 & /**/) override
            {
                if (me->HealthBelowPct(50) && !_attumenDied)
                {
                    _events.Reset();

                    Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID);
                    if (attumen)
                    {
                        attumen->GetAI()->DoAction(ACTION_DISMOUNT_MIDNIGHT);
                        me->RemoveAurasDueToSpell(SPELL_FORCE_RIDE_VEHICLE);
                        attumen->RemoveAurasDueToSpell(SPELL_MOUNTED);
                    }

                    me->CastSpell(me, SPELL_RIDERLESS, true);
                    me->SetReactState(REACT_PASSIVE);
                    _events.ScheduleEvent(EVENT_MEZAIR, 2500, PHASE_TWO);
                }
            }

            void ExecuteEvent(uint32 eventId) override
            {
                switch (eventId)
                {
                    case EVENT_FOLLOW_TARGET:
                    {
                        if (me->GetVictim())
                            me->GetMotionMaster()->MoveChase(me->GetVictim());

                        _events.ScheduleEvent(EVENT_FOLLOW_TARGET, 500);
                        break;
                    }
                    case EVENT_STAMP:
                    {
                        DoCastVictim(SPELL_STAMP);
                        _events.ScheduleEvent(EVENT_STAMP, Seconds(urand(8, 12)), PHASE_ONE);
                        break;
                    }

                    case EVENT_INTAGIBLE_PRESSENCE:
                    {
                        if (Creature* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID))
                            attumen->AI()->Talk(SAY_INTAGIBLE_PRESSENCE);
                        DoCast(SPELL_INTANGIBLE_PRESENCE);
                        _events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, Seconds(30), PHASE_ONE);
                        break;
                    }

                    case EVENT_MIGHTY_STOMP:
                    {
                        if (roll_chance_i(50))
                        {
                            if (auto* attumen = ObjectAccessor::GetCreature(*me, _attumenGUID))
                                attumen->AI()->Talk(SAY_MIGHTY_STOMP);

                        }
                        DoCast(SPELL_MIGHTY_STOMP);
                        _events.ScheduleEvent(EVENT_MIGHTY_STOMP, Seconds(65), PHASE_ONE);
                        break;
                    }

                    case EVENT_MEZAIR:
                    {
                        me->GetMotionMaster()->MovePoint(POINT_MEZAIR, me->GetRandomPoint(CenterOfRoom, 10.f));
                        break;
                    }

                    case EVENT_SPECTRAL_CHARGE:
                    {
                        _events.CancelEvent(EVENT_FOLLOW_TARGET);
                        me->GetMotionMaster()->MovePoint(POINT_SPECTRAL_CHARGE, me->GetRandomPoint(CenterOfRoom, 10.f));
                        break;
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                    ExecuteEvent(eventId);

                DoMeleeAttackIfReady();
            }

            private:
                SummonList _summons;
                EventMap _events;
                ObjectGuid _attumenGUID;
                uint8 _chargesDone;
                bool _attumenDied, _firstCharge;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_kara_midnight_AI(creature);
        }
};

class npc_kara_intagible_presence : public CreatureScript
{
    public:
        npc_kara_intagible_presence() : CreatureScript("npc_kara_intagible_presence")
        {}

        struct npc_kara_intagible_presence_AI : public ScriptedAI
        {
            explicit npc_kara_intagible_presence_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->CastSpell(me, SPELL_GHOST_VISUAL, true);
                me->CastSpell(me, SPELL_GHOST_VISUAL_2, true);
                me->CastSpell(me, SPELL_GHOSTLY_COSMETIC, true);
            }

            void Reset() override
            {
                _events.Reset();
                me->CastSpell(me, SPELL_GHOST_VISUAL, true);
                me->CastSpell(me, SPELL_GHOST_VISUAL_2, true);
                me->CastSpell(me, SPELL_GHOSTLY_COSMETIC, true);
            }

            void IsSummonedBy(Unit* /**/) override
            {
                _events.ScheduleEvent(EVENT_MOUNT_VEHICLE, 1000);
                _events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, 3000);
            }

            void SetGUID(ObjectGuid guid, int32 id) override
            {
                if (id == DATA_PASSENGER)
                    _passengerGUID = guid;
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTAGIBLE_PRESSENCE:
                        {
                            DoCastSelf(SPELL_INTANGIBLE_PRESENCE_DMG, true);
                            _events.ScheduleEvent(EVENT_INTAGIBLE_PRESSENCE, 2000);
                            break;
                        }

                        case EVENT_MOUNT_VEHICLE:
                        {
                            if (Player* ptr = ObjectAccessor::GetPlayer(*me, _passengerGUID))
                            {
                                if (Vehicle* vec = ptr->GetVehicleKit())
                                {
                                    if (!vec->IsVehicleInUse())
                                    {
                                        me->EnterVehicle(ptr);
                                    }
                                    else
								    {
                                        me->DespawnOrUnsummon();
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }

            private:
                ObjectGuid _passengerGUID;
                EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_kara_intagible_presence_AI(creature);
        }
};

class spell_attumen_riderless : public SpellScriptLoader
{
    public:
        spell_attumen_riderless() : SpellScriptLoader("spell_attumen_riderless")
        {}

        class spell_riderless_AuraScript : public AuraScript
        {
            public:
                PrepareAuraScript(spell_riderless_AuraScript);

                void HandlePeriodic(AuraEffect const* /**/)
                {
                    if (!GetUnitOwner())
                        return;

                    if (GetUnitOwner()->IsFullHealth())
                        GetUnitOwner()->GetAI()->DoAction(ACTION_FULL_HEALTH);
                }

                void Register() override
                {
                    OnEffectPeriodic += AuraEffectPeriodicFn(spell_riderless_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH);
                }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_riderless_AuraScript();
        }
};

class spell_attumen_intagible_presence : public SpellScriptLoader
{
    public:
        spell_attumen_intagible_presence() : SpellScriptLoader("spell_attumen_intagible_presence")
        {}

        class spell_intagible_presence_SpellScript : public SpellScript
        {
            public:
                PrepareSpellScript(spell_intagible_presence_SpellScript);

                void HandleOnCast()
                {
                    if (!GetCaster())
                        return;

                    if (GetCaster()->GetTypeId() == TYPEID_PLAYER)
                        return;

                    Unit* target = GetCaster()->GetAI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true);

                    if (target)
                    {
                        auto* ghost = GetCaster()->SummonCreature(NPC_INTAGIBLE_PRESENCE, target->GetPosition());

                        if (ghost)
                            ghost->GetAI()->SetGUID(target->GetGUID(), DATA_PASSENGER);
                    }
                }

                void Register()
                {
                    OnCast += SpellCastFn(spell_intagible_presence_SpellScript::HandleOnCast);
                }
        };

        class spell_intagible_presence_AuraScript : public AuraScript
        {
            public:
                PrepareAuraScript(spell_intagible_presence_AuraScript);

                void HandleDispel(DispelInfo* /**/)
                {
                    if (!GetUnitOwner())
                        return;

                    Vehicle* vec = GetUnitOwner()->GetVehicleKit();

                    if (!vec)
                        return;

                    if (!vec->IsVehicleInUse())
                        GetUnitOwner()->CastSpell(GetUnitOwner(), SPELL_INTANGIBLE_PRESENCE_DISPEL, true);
                    else
                    {
                        Creature* ghost = GetUnitOwner()->FindNearestCreature(NPC_INTAGIBLE_PRESENCE, 5.f, true);

                        if (ghost)
                            ghost->DespawnOrUnsummon();
                    }
                }

                void Register() override
                {
                    OnDispel += AuraDispelFn(spell_intagible_presence_AuraScript::HandleDispel);
                }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_intagible_presence_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_intagible_presence_AuraScript();
        }
};
void AddSC_boss_attumen_new()
{
    new boss_attumen_new();
    new npc_kara_midnight();
    new npc_kara_intagible_presence();
    new spell_attumen_riderless();
    new spell_attumen_intagible_presence();
}
