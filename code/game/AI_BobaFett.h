#define BOBA_MISSILE_ROCKET 0
#define BOBA_MISSILE_LASER	1
#define BOBA_MISSILE_DART	2
#define BOBA_MISSILE_VIBROBLADE	3

extern void	Boba_FireWristMissile( gentity_t *self, int whichMissile );
extern void Boba_EndWristMissile( gentity_t *self, int whichMissile );

typedef struct wristWeapon_s {
	int theMissile;
	int dummyForcePower;
	int whichWeapon;
	qboolean altFire;
	int maxShots;
	int animTimer;
	int animDelay;
	int fireAnim;
	qboolean fullyCharged;
	qboolean leftBolt;
	qboolean hold;
} wristWeapon_t;
