#include "universe_util.h"
#include "missile_generic.h"
#include "unit_generic.h"
#include "vegastrike.h"
#include "vs_globals.h"
#include "configxml.h"
#include "images.h"
#include "collection.h"
#include "star_system_generic.h"
void StarSystem::UpdateMissiles() {  
  if (!dischargedMissiles.empty()) {
    Unit * un;
    for (un_iter ui=getUnitList().createIterator();
	 NULL!= (un=(*ui));
	 ++ui) {
      dischargedMissiles.back()->ApplyDamage (un);
    }
    delete dischargedMissiles.back();
    dischargedMissiles.pop_back();
  }
  
}
void MissileEffect::ApplyDamage (Unit * smaller) {
  float rad =(smaller->Position().Cast()-pos).Magnitude()-smaller->rSize();
  float orig = rad;
  if(rad==0) rad =0.001;
  rad=rad*rad;
  if (smaller->isUnit()!=MISSILEPTR&&rad<radius*radius) {
	if(rad<(radialmultiplier*radialmultiplier)){
	  rad = (radialmultiplier*radialmultiplier*radialmultiplier*radialmultiplier/((2*radialmultiplier*radialmultiplier)-(orig*orig))); 
	  /*
	  contrived formula to create paraboloid falloff rather than quadratic peaking at 2x damage at origin
	  rad = radmul^4/(2radmul^2-orig^2)
	  */
	}
    rad=rad/(radialmultiplier*radialmultiplier); // where radialmultiplier is radius of point with 0 falloff
	
	if( (damage>0)) {
      Vector norm (pos-smaller->Position().Cast());
      norm.Normalize();
	  UniverseUtil::IOmessage(0,"game","all",string("dealt ")+XMLSupport::tostring(damage/rad)+string(" damage from ")+XMLSupport::tostring(orig)+string(" meters ")+XMLSupport::tostring(damage)+string(" damage and "+XMLSupport::tostring(radialmultiplier)+string(" rad mult"))); 
      //divide effects by r^2
      smaller->ApplyDamage (pos,norm,damage/rad,smaller,GFXColor(1,1,1,1),NULL,phasedamage>0?phasedamage/rad:0);
    }
    if (damage<0||phasedamage<0) {
      smaller->leach (1,phasedamage<0?-phasedamage:1,damage<0?-damage:0);
    }  
  }
}

float Missile::ExplosionRadius() {
  return radial_effect;
}

void StarSystem::AddMissileToQueue(MissileEffect * me) {
  dischargedMissiles.push_back (me);
}
void Missile::Discharge() {
  if (!discharged)
    _Universe->activeStarSystem()->AddMissileToQueue(new MissileEffect(Position().Cast(),damage,phasedamage,radial_effect,radial_multiplier));
  discharged =true;
}
void Missile::Kill (bool erase) {
  Discharge();
  Unit::Kill(erase);
}
void Missile::reactToCollision (Unit * smaller, const QVector & biglocation, const Vector & bignormal, const QVector & smalllocation, const Vector & smallnormal, float dist) {
  static bool doesmissilebounce  = XMLSupport::parse_bool (vs_config->getVariable("physics","missile_bounce","false"));
  if (doesmissilebounce) {

    Unit::reactToCollision (smaller,biglocation,bignormal,smalllocation,smallnormal,dist);
  }
  Discharge();
  if (!killed)
    DealDamageToHull (smalllocation.Cast(),hull+1);//should kill, applying addmissile effect
  
}

Unit * getNearestTarget (Unit *me) {
  QVector pos (me->Position());
  Unit * un=NULL;
  Unit * targ=NULL;
  double minrange=FLT_MAX;
  for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();
       (un=(*i));
       ++i) {
         if (un==me)
           continue;
         if (un->isUnit()!=UNITPTR) {
            continue;     
         }
         double temp= (un->Position()-pos).Magnitude()-un->rSize();
         if (targ==NULL) {
            targ = un;
            minrange = temp;
         }else {
           if (temp<minrange) {
              targ = un;
           }
         }
       }
    return targ;
}
void Missile::UpdatePhysics (const Transformation &trans, const Matrix &transmat, const Vector & CumulativeVelocity, bool ResolveLast, UnitCollection *uc){
    Unit * targ;
	if ((targ=(Unit::Target()))) {
      if (rand()/((float)RAND_MAX)<((float)targ->GetImageInformation().ecm)*SIMULATION_ATOM/32768){
	Target (this);//go wild
      }
    }
    if (retarget==-1){
      if (targ) {
        retarget=1;
      }else {
        retarget=0;
      }
    }
    if (retarget&&targ==NULL) {
      Target (getNearestTarget (this));
    }
    Unit::UpdatePhysics (trans, transmat, CumulativeVelocity, ResolveLast, uc);
    this->time-=SIMULATION_ATOM;
	float checker = targ->querySphere (Position()-(SIMULATION_ATOM*GetVelocity()),Position(),rSize());
    if (NULL!=targ) {
      if (checker||((Position()-targ->Position()).Magnitude()-targ->rSize()-rSize()<detonation_radius)) {
		  Discharge();
		  time=-1;
	//Vector norm;
	//float dist;
	/*** WARNING COLLISION STUFF... TO FIX FOR SERVER SIDE SOMEDAY ***
	if ((targ)->queryBoundingBox (Position(),detonation_radius+rSize())) {
	  Discharge();
	  time=-1;
	}
	*/
      }
    }
    if (time<0) {
      DealDamageToHull (Vector(.1,.1,.1),hull+1);
    }
  }
