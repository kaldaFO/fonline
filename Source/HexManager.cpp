#include "StdAfx.h"
#include "HexManager.h"
#include "ResourceManager.h"

#ifdef FONLINE_MAPPER
#include "CritterData.h"
#include "CritterManager.h"
#endif

#if defined(FONLINE_CLIENT) || defined(FONLINE_MAPPER)
#include "Script.h"
#endif

/************************************************************************/
/* FIELD                                                                */
/************************************************************************/

void Field::Clear()
{
	Crit=NULL;
	DeadCrits.clear();
	ScrX=0;
	ScrY=0;
	TileId=0;
	RoofId=0;
	RoofNum=0;
	Items.clear();
	ScrollBlock=false;
	IsWall=false;
	IsWallSAI=false;
	IsWallTransp=false;
	IsScen=false;
	IsExitGrid=false;
	LightType=0;
	IsNotPassed=false;
	IsNotRaked=false;
	IsNoLight=false;
}

void Field::AddItem(ItemHex* item)
{
	Items.push_back(item);
	ProcessCache();
}

void Field::EraseItem(ItemHex* item)
{
	ItemHexVecIt it=std::find(Items.begin(),Items.end(),item);
	if(it!=Items.end())
	{
		Items.erase(it);
		ProcessCache();
	}
}

void Field::ProcessCache()
{
	IsWall=false;
	IsWallSAI=false;
	IsWallTransp=false;
	IsScen=false;
	IsExitGrid=false;
	LightType=0;
	IsNotPassed=(Crit!=NULL);
	IsNotRaked=false;
	IsNoLight=false;
	ScrollBlock=false;
	for(ItemHexVecIt it=Items.begin(),end=Items.end();it!=end;++it)
	{
		ItemHex* item=*it;
		WORD pid=item->GetProtoId();
		if(item->IsWall())
		{
			IsWall=true;
			IsWallTransp=item->IsLightThru();
			LightType=item->Proto->LightType;
			if(pid==SP_SCEN_IBLOCK) IsWallSAI=true;
		}
		else if(item->IsScenOrGrid())
		{
			IsScen=true;
			if(pid==SP_WALL_BLOCK || pid==SP_WALL_BLOCK_LIGHT) IsWall=true;
			else if(pid==SP_GRID_EXITGRID) IsExitGrid=true;
		}
		if(item->IsDoor() && item->LockerIsClose())
		{
			IsNotPassed=true;
			IsNotRaked=true;
			IsNoLight=true;
		}
		if(!item->IsPassed()) IsNotPassed=true;
		if(!item->IsRaked()) IsNotRaked=true;
		if(pid==SP_MISC_SCRBLOCK) ScrollBlock=true;
		if(!item->IsLightThru()) IsNoLight=true;
	}
}

/************************************************************************/
/* HEX FIELD                                                            */
/************************************************************************/

HexManager::HexManager()
{
	viewField=NULL;
	tileVB=NULL;
	tileCntPrep=0;
	sprMngr=NULL;
	isShowHex=false;
	roofSkip=false;
	rainCapacity=0;
	chosenId=0;
	critterContourCrId=0;
	crittersContour=Sprite::ContourNone;
	critterContour=Sprite::ContourNone;
	maxHexX=0;
	maxHexY=0;
	hexField=NULL;
	hexToDraw=NULL;
	hexTrack=NULL;
	hexLight=NULL;
	hTop=0;
	hBottom=0;
	wLeft=0;
	wRight=0;
	isShowCursor=false;
	drawCursorX=0;
	cursorPrePic=0;
	cursorPostPic=0;
	cursorXPic=0;
	cursorX=0;
	cursorY=0;
	ZeroMemory((void*)&AutoScroll,sizeof(AutoScroll));
	lightPointsCount=0;
	SpritesCanDrawMap=false;
	dayTime[0]=300; dayTime[1]=600;  dayTime[2]=1140; dayTime[3]=1380;
	dayColor[0]=18; dayColor[1]=128; dayColor[2]=103; dayColor[3]=51;
	dayColor[4]=18; dayColor[5]=128; dayColor[6]=95;  dayColor[7]=40;
	dayColor[8]=53; dayColor[9]=128; dayColor[10]=86; dayColor[11]=29;
}

bool HexManager::Init(SpriteManager* sm)
{
	WriteLog("Hex field initialization...\n");

	if(!sm)
	{
		WriteLog("Sprite manager nullptr.\n");
		return false;
	}
	sprMngr=sm;

	if(!ItemMngr.IsInit())
	{
		WriteLog("Item manager is not init.\n");
		return false;
	}

	isShowTrack=false;
	curPidMap=0;
	curMapTime=-1;
	curHashTiles=0;
	curHashWalls=0;
	curHashScen=0;
	isShowCursor=false;
	cursorX=0;
	cursorY=0;
	chosenId=0;
	ZeroMemory((void*)&AutoScroll,sizeof(AutoScroll));
	maxHexX=0;
	maxHexY=0;

#ifdef FONLINE_MAPPER
	ClearSelTiles();
	CurProtoMap=NULL;
#endif

	WriteLog("Hex field initialization success.\n");
	return true;
}

bool HexManager::ReloadSprites(SpriteManager* sm)
{
	if(!sm) sm=sprMngr;
	if(!sm) return false;
	if(!(hexWhite=sm->LoadSprite("hex.frm",PT_ART_MISC))) return false;
	if(!(hexBlue=sm->LoadSprite("hexb.frm",PT_ART_MISC))) return false;
	if(!(cursorPrePic=sm->LoadSprite("move_pre.png",PT_ART_MISC))) return false;
	if(!(cursorPostPic=sm->LoadSprite("move_post.png",PT_ART_MISC))) return false;
	if(!(cursorXPic=sm->LoadSprite("move_x.png",PT_ART_MISC))) return false;
	if(!(picRainDrop=sm->LoadSprite("drop.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[0]=sm->LoadSprite("adrop1.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[1]=sm->LoadSprite("adrop2.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[2]=sm->LoadSprite("adrop3.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[3]=sm->LoadSprite("adrop4.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[4]=sm->LoadSprite("adrop5.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[5]=sm->LoadSprite("adrop6.png",PT_ART_MISC))) return false;
	if(!(picRainDropA[6]=sm->LoadSprite("adrop7.png",PT_ART_MISC))) return false;
	if(!(picTrack1=sm->LoadSprite("track1.png",PT_ART_MISC))) return false;
	if(!(picTrack2=sm->LoadSprite("track2.png",PT_ART_MISC))) return false;
	return true;
}

void HexManager::Clear()
{
	WriteLog("Hex field finish...\n");

	mainTree.Clear();
	roofRainTree.Clear();
	roofTree.Clear();

	for(DropVecIt it=rainData.begin(),end=rainData.end();it!=end;++it)
		SAFEDEL(*it);
	rainData.clear();

	for(int i=0,j=hexItems.size();i<j;i++)
		hexItems[i]->Release();
	hexItems.clear();

	for(int hx=0;hx<maxHexX;hx++)
		for(int hy=0;hy<maxHexY;hy++)
			GetField(hx,hy).Clear();

	for(OneSurfVecIt it=tilePrepSurf.begin(),end=tilePrepSurf.end();it!=end;++it)
		delete *it;
	tilePrepSurf.clear();

	SAFEREL(tileVB);
	SAFEDELA(viewField);
	ResizeField(0,0);

	chosenId=0;
	WriteLog("Hex field finish complete.\n");
}

void HexManager::PlaceCarBlocks(WORD hx, WORD hy, ProtoItem* proto_item)
{
	if(!proto_item) return;

	bool raked=FLAG(proto_item->Flags,ITEM_SHOOT_THRU);
	BYTE dir;
	int steps;
	for(int i=0;i<CAR_MAX_BLOCKS;i++)
	{
		dir=proto_item->MiscEx.Car.GetBlockDir(i);
		if(dir>5) return;

		i++;
		steps=proto_item->MiscEx.Car.GetBlockDir(i);

		for(int j=0;j<steps;j++)
		{
			MoveHexByDir(hx,hy,dir,maxHexX,maxHexY);
			GetField(hx,hy).IsNotPassed=true;
			if(!raked) GetField(hx,hy).IsNotRaked=true;
		}
	}
}

void HexManager::ReplaceCarBlocks(WORD hx, WORD hy, ProtoItem* proto_item)
{
	if(!proto_item) return;

	bool raked=FLAG(proto_item->Flags,ITEM_SHOOT_THRU);
	BYTE dir;
	int steps;
	for(int i=0;i<CAR_MAX_BLOCKS;i++)
	{
		dir=proto_item->MiscEx.Car.GetBlockDir(i);
		if(dir>5) return;

		i++;
		steps=proto_item->MiscEx.Car.GetBlockDir(i);

		for(int j=0;j<steps;j++)
		{
			MoveHexByDir(hx,hy,dir,maxHexX,maxHexY);
			GetField(hx,hy).ProcessCache();
		}
	}
}

bool HexManager::AddItem(DWORD id, WORD pid, WORD hx, WORD hy, bool is_added, Item::ItemData* data)
{
	if(!id)
	{
		WriteLog(__FUNCTION__" - Item id is zero.\n");
		return false;
	}

	if(!IsMapLoaded())
	{
		WriteLog(__FUNCTION__" - Map is not loaded.");
		return false;
	}

	if(hx>=maxHexX || hy>=maxHexY)
	{
		WriteLog(__FUNCTION__" - Position hx<%u> or hy<%u> error value.\n",hx,hy);
		return false;
	}

	ProtoItem* proto=ItemMngr.GetProtoItem(pid);
	if(!proto)
	{
		WriteLog(__FUNCTION__" - Proto not found<%u>.\n",pid);
		return false;
	}

	// Change
	ItemHex* item_old=GetItemById(id);
	if(item_old)
	{
		if(item_old->IsFinishing()) item_old->StopFinishing();
		if(item_old->GetProtoId()==pid && item_old->GetHexX()==hx && item_old->GetHexY()==hy)
		{
			ChangeItem(id,*data);
			return true;
		}
		else DeleteItem(item_old);
	}

	// Parse
	Field& f=GetField(hx,hy);
	ItemHex* item=new ItemHex(id,proto,data,hx,hy,0,0,0,&f.ScrX,&f.ScrY);
	if(is_added) item->SetShowAnim();
	PushItem(item);

	// Check ViewField size
	if(ProcessHexBorders(item->Anim->GetSprId(0),0,0))
	{
		// Resize
		hVisible=VIEW_HEIGHT+hTop+hBottom;
		wVisible=VIEW_WIDTH+wLeft+wRight;
		delete[] viewField;
		viewField=new ViewField[hVisible*wVisible];
		RefreshMap();
	}
	else
	{
		// Draw
		if(GetHexToDraw(hx,hy) && !item->IsHidden() &&  item->Proto->TransVal<0xFF)
		{
			Sprite& spr=mainTree.InsertSprite(item->IsFlat()?DRAW_ORDER_ITEM_FLAT(item->IsScenOrGrid()):DRAW_ORDER_ITEM(item->GetPos()),
				f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&item->SprId,&item->ScrX,&item->ScrY,&item->Alpha,
				!item->IsNoLightInfluence()?GetLightHex(hx,hy):NULL,&item->SprDrawValid);
			item->SprDraw=&spr;
			item->SetEffects(&spr);
		}

		if(item->IsLight()) RebuildLight();
		else if(!item->IsLightThru())
		{
			CritterCl* chosen=GetChosen();
			if(chosen && CheckDist(chosen->GetHexX(),chosen->GetHexY(),item->GetHexX(),item->GetHexY(),4)) RebuildLight();
		}
	}

	return true;
}

void HexManager::ChangeItem(DWORD id, const Item::ItemData& data)
{
	ItemHex* item=GetItemById(id);
	if(!item) return;

	ProtoItem* proto=item->Proto;
	Item::ItemData old_data=item->Data;
	item->Data=data;

	if(old_data.PicMapHash!=data.PicMapHash) item->RefreshAnim();
	if(proto->IsDoor() || proto->IsContainer())
	{
		WORD old_cond=old_data.Locker.Condition;
		WORD new_cond=data.Locker.Condition;
		if(!FLAG(old_cond,LOCKER_ISOPEN) && FLAG(new_cond,LOCKER_ISOPEN)) item->SetAnimFromStart();
		if(FLAG(old_cond,LOCKER_ISOPEN) && !FLAG(new_cond,LOCKER_ISOPEN)) item->SetAnimFromEnd();
	}

	CritterCl* chosen=GetChosen();
	if(item->IsLight() || (chosen && CheckDist(chosen->GetHexX(),chosen->GetHexY(),item->GetHexX(),item->GetHexY(),4))) RebuildLight();
	GetField(item->GetHexX(),item->GetHexY()).ProcessCache();
}

void HexManager::FinishItem(DWORD id, bool is_deleted)
{
	if(!id)
	{
		WriteLog(__FUNCTION__" - Item zero id.\n");
		return;
	}

	ItemHex* item=GetItemById(id);
	if(!item)
	{
		WriteLog(__FUNCTION__" - Item<%d> not found.\n",id);
		return;
	}

	item->Finish();
	if(is_deleted) item->SetHideAnim();
}

ItemHexVecIt HexManager::DeleteItem(ItemHex* item, bool with_delete /* = true */)
{
	WORD pid=item->GetProtoId();
	WORD hx=item->HexX;
	WORD hy=item->HexY;

	if(item->Proto->IsCar()) ReplaceCarBlocks(item->HexX,item->HexY,item->Proto);
	if(item->SprDrawValid) item->SprDraw->Unvalidate();

	ItemHexVecIt it=std::find(hexItems.begin(),hexItems.end(),item);
	if(it!=hexItems.end()) it=hexItems.erase(it);
	GetField(hx,hy).EraseItem(item);

	if(item->IsLight()) RebuildLight();
	else if(!item->IsLightThru())
	{
		CritterCl* chosen=GetChosen();
		if(chosen && CheckDist(chosen->GetHexX(),chosen->GetHexY(),item->GetHexX(),item->GetHexY(),4)) RebuildLight();
	}

	if(with_delete) item->Release();
	return it;
}

void HexManager::ProcessItems()
{
	for(ItemHexVecIt it=hexItems.begin();it!=hexItems.end();)
	{
		ItemHex* item=*it;
		item->Process();

		if(item->IsDynamicEffect() && !item->IsFinishing())
		{
			WordPair step=item->GetEffectStep();
			if(item->GetHexX()!=step.first || item->GetHexY()!=step.second)
			{
				WORD hx=item->GetHexX();
				WORD hy=item->GetHexY();
				int x,y;
				GetHexOffset(hx,hy,step.first,step.second,x,y);
				item->EffOffsX-=x;
				item->EffOffsY-=y;
				Field& f=GetField(hx,hy);
				Field& f_=GetField(step.first,step.second);
				f.EraseItem(item);
				f_.AddItem(item);
				item->HexX=step.first;
				item->HexY=step.second;
				if(item->SprDrawValid) item->SprDraw->Unvalidate();

				item->HexScrX=&f_.ScrX;
				item->HexScrY=&f_.ScrY;
				if(GetHexToDraw(step.first,step.second)) item->SprDraw=&mainTree.InsertSprite(DRAW_ORDER_ITEM(item->GetPos()),
					f_.ScrX+HEX_OX,f_.ScrY+HEX_OY,0,&item->SprId,&item->ScrX,&item->ScrY,
					&item->Alpha,!item->IsNoLightInfluence()?GetLightHex(step.first,step.second):NULL,&item->SprDrawValid);
				item->SetAnimOffs();
			}
		}

		if(item->IsFinish()) it=DeleteItem(item);
		else ++it;
	}
}

bool ItemCompScen(ItemHex* o1, ItemHex* o2){return o1->IsScenOrGrid() && !o2->IsScenOrGrid();}
bool ItemCompWall(ItemHex* o1, ItemHex* o2){return o1->IsWall() && !o2->IsWall();}
//bool ItemCompSort(ItemHex* o1, ItemHex* o2){return o1->IsWall() && !o2->IsWall();}
void HexManager::PushItem(ItemHex* item)
{
	hexItems.push_back(item);
	WORD hx=item->HexX;
	WORD hy=item->HexY;
	Field& f=GetField(hx,hy);
	item->HexScrX=&f.ScrX;
	item->HexScrY=&f.ScrY;
	f.AddItem(item);
	if(item->Proto->IsCar()) PlaceCarBlocks(hx,hy,item->Proto);
	// Sort
	std::sort(f.Items.begin(),f.Items.end(),ItemCompScen);
	std::sort(f.Items.begin(),f.Items.end(),ItemCompWall);
}

ItemHex* HexManager::GetItem(WORD hx, WORD hy, WORD pid)
{
	if(!IsMapLoaded() || hx>=maxHexX || hy>=maxHexY) return NULL;

	for(ItemHexVecIt it=GetField(hx,hy).Items.begin(),end=GetField(hx,hy).Items.end();it!=end;++it)
	{
		ItemHex* item=*it;
		if(item->GetProtoId()==pid) return item;
	}
	return NULL;
}

ItemHex* HexManager::GetItemById(WORD hx, WORD hy, DWORD id)
{
	if(!IsMapLoaded() || hx>=maxHexX || hy>=maxHexY) return NULL;

	for(ItemHexVecIt it=GetField(hx,hy).Items.begin(),end=GetField(hx,hy).Items.end();it!=end;++it)
	{
		ItemHex* item=*it;
		if(item->GetId()==id) return item;
	}
	return NULL;
}

ItemHex* HexManager::GetItemById(DWORD id)
{
	for(ItemHexVecIt it=hexItems.begin(),end=hexItems.end();it!=end;++it)
	{
		ItemHex* item=*it;
		if(item->GetId()==id) return item;
	}
	return NULL;
}

void HexManager::GetItems(WORD hx, WORD hy, ItemHexVec& items)
{
	if(!IsMapLoaded()) return;

	Field& f=GetField(hx,hy);
	for(int i=0,j=f.Items.size();i<j;i++)
	{
		if(std::find(items.begin(),items.end(),f.Items[i])==items.end()) items.push_back(f.Items[i]);
	}
}

INTRECT HexManager::GetRectForText(WORD hx, WORD hy)
{
	if(!IsMapLoaded()) return INTRECT();
	Field& f=GetField(hx,hy);

	// Critters first
	if(f.Crit) return f.Crit->GetTextRect();
	else if(f.DeadCrits.size()) return f.DeadCrits[0]->GetTextRect();

	// Scenery
	for(int i=0,j=f.Items.size();i<j;i++)
	{
		ItemHex* item=f.Items[i];
		if(!item->IsScenOrGrid()) continue;
		SpriteInfo* si=sprMngr->GetSpriteInfo(item->SprId);
		if(!si) continue;
		return INTRECT(0,0,si->Width,si->Height);
	}
	return INTRECT(0,0,0,0);
}

bool HexManager::RunEffect(WORD eff_pid, WORD from_hx, WORD from_hy, WORD to_hx, WORD to_hy)
{
	if(!IsMapLoaded()) return false;
	if(from_hx>=maxHexX || from_hy>=maxHexY || to_hx>=maxHexX || to_hy>=maxHexY)
	{
		WriteLog(__FUNCTION__" - Incorrect value of from_x<%d> or from_y<%d> or to_x<%d> or to_y<%d>.\n",from_hx,from_hy,to_hx,to_hy);
		return false;
	}

	ProtoItem* proto=ItemMngr.GetProtoItem(eff_pid);
	if(!proto)
	{
		WriteLog(__FUNCTION__" - Proto not found, pid<%d>.\n",eff_pid);
		return false;
	}

	Field& f=GetField(from_hx,from_hy);
	ItemHex* item=new ItemHex(0,proto,NULL,from_hx,from_hy,GetFarDir(from_hx,from_hy,to_hx,to_hy),0,0,&f.ScrX,&f.ScrY);
	item->SetAnimFromStart();

	float sx=0;
	float sy=0;
	int dist=0;

	if(from_hx!=to_hx || from_hy!=to_hy)
	{
		item->EffSteps.push_back(WordPairVecVal(from_hx,from_hy));
		TraceBullet(from_hx,from_hy,to_hx,to_hy,0,0.0f,NULL,false,NULL,0,NULL,NULL,&item->EffSteps,false);
		int x,y;
		GetHexOffset(from_hx,from_hy,to_hx,to_hy,x,y);
		y+=Random(5,25); // Center of body
		GetStepsXY(sx,sy,0,0,x,y);
		dist=DistSqrt(0,0,x,y);
	}

	item->SetEffect(sx,sy,dist);

	f.AddItem(item);
	hexItems.push_back(item);

	if(GetHexToDraw(from_hx,from_hy)) item->SprDraw=&mainTree.InsertSprite(DRAW_ORDER_ITEM(item->GetPos()),
		f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&item->SprId,&item->ScrX,&item->ScrY,
		&item->Alpha,!item->IsNoLightInfluence()?GetLightHex(from_hx,from_hy):NULL,&item->SprDrawValid);

	return true;
}

void HexManager::ProcessRain()
{
	if(!rainCapacity) return;

	static DWORD last_tick=Timer::FastTick();
	DWORD delta=Timer::FastTick()-last_tick;
	if(delta<=RAIN_TICK) return;

	for(DropVecIt it=rainData.begin(),end=rainData.end();it!=end;++it)
	{
		Drop* cur_drop=(*it);
		if(cur_drop->CurSprId==picRainDrop)
		{
			if((cur_drop->OffsY+=RAIN_SPEED)>=cur_drop->GroundOffsY)
				cur_drop->CurSprId=picRainDropA[cur_drop->DropCnt];
		}
		else
		{
			cur_drop->CurSprId=picRainDropA[cur_drop->DropCnt++];

			if(cur_drop->DropCnt>6)
			{
				cur_drop->CurSprId=picRainDrop;
				cur_drop->DropCnt=0;
				cur_drop->OffsX=random(10)-random(10);
				cur_drop->OffsY=(100+random(100))*-1;
			}
		}
	}
	
	last_tick=Timer::FastTick();
}

void HexManager::SetCursorPos(int x, int y, bool show_steps, bool refresh)
{
	WORD hx,hy;
	if(GetHexPixel(x,y,hx,hy))
	{
		Field& f=GetField(hx,hy);
		cursorX=f.ScrX+1-1;
		cursorY=f.ScrY-1-1;
		if(f.IsNotPassed)
		{
			drawCursorX=-1;
		}
		else
		{
			static int last_cur_x=0;
			static WORD last_hx=0,last_hy=0;
			if(refresh || hx!=last_hx || hy!=last_hy)
			{
				CritterCl* chosen=GetChosen();
				if(chosen)
				{
					bool is_tb=chosen->IsTurnBased();
					if(chosen->IsLife() && (!is_tb || chosen->GetAllAp()>0))
					{
						ByteVec steps;
						int res=FindStep(chosen->GetHexX(),chosen->GetHexY(),hx,hy,steps);
						if(res!=FP_OK) drawCursorX=-1;
						else if(!is_tb) drawCursorX=(show_steps?steps.size():0);
						else
						{
							drawCursorX=steps.size();
							if(!show_steps && drawCursorX>chosen->GetAllAp()) drawCursorX=-1;
						} 
					}
					else
					{
						drawCursorX=-1;
					}
				}
				last_hx=hx;
				last_hy=hy;
				last_cur_x=drawCursorX;
			}
			else
			{
				drawCursorX=last_cur_x;
			}
		}
	}
}

void HexManager::DrawCursor(DWORD spr_id)
{
	if(OptHideCursor || !isShowCursor) return;
	SpriteInfo* si=sprMngr->GetSpriteInfo(spr_id);
	if(!si) return;
	sprMngr->DrawSpriteSize(spr_id,(cursorX+CmnScrOx)/ZOOM,(cursorY+CmnScrOy)/ZOOM,si->Width/ZOOM,si->Height/ZOOM,true,false);
}

void HexManager::DrawCursor(const char* text)
{
	if(OptHideCursor || !isShowCursor) return;
	int x=(cursorX+CmnScrOx)/ZOOM;
	int y=(cursorY+CmnScrOy)/ZOOM;
	sprMngr->DrawStr(INTRECT(x,y,x+32,y+16),text,FT_CENTERX|FT_CENTERY,COLOR_TEXT_WHITE);
}

void HexManager::RebuildMap(int rx, int ry)
{
	if(!viewField) return;
	if(rx<0 || ry<0 || rx>=maxHexX || ry>=maxHexY) return;

	InitView(rx,ry);

	// Set to draw hexes
	ClearHexToDraw();

	int ty;
	int y2=0;
	int vpos;
	for(ty=0;ty<hVisible;ty++)
	{
		for(int tx=0;tx<wVisible;tx++)
		{
			vpos=y2+tx;

			int hx=viewField[vpos].HexX;
			int hy=viewField[vpos].HexY;

			if(hx<0 || hy<0 || hx>=maxHexX || hy>=maxHexY) continue;

			GetHexToDraw(hx,hy)=true;
			Field& f=GetField(hx,hy);
			f.ScrX=viewField[vpos].ScrX;
			f.ScrY=viewField[vpos].ScrY;
		}
		y2+=wVisible;
	}

	// Light
	RebuildLight();

	// Tiles, roof
	RebuildTiles();
	RebuildRoof();

	// Erase old sprites
	mainTree.Unvalidate();
	roofRainTree.Unvalidate();

	for(DropVecIt it=rainData.begin(),end=rainData.end();it!=end;++it)
		delete (*it);
	rainData.clear();

	sprMngr->EggNotValid();

	// Begin generate new sprites
	y2=0;
	for(ty=0;ty<hVisible;ty++)
	{
		for(int x=0;x<wVisible;x++)
		{
			vpos=y2+x;
			int ny=viewField[vpos].HexY;
			int nx=viewField[vpos].HexX;
			if(ny<0 || nx<0 || nx>=maxHexX || ny>=maxHexY) continue;

			Field& f=GetField(nx,ny);

			// Track
			if(isShowTrack && GetHexTrack(nx,ny))
			{
				DWORD spr_id=(GetHexTrack(nx,ny)==1?picTrack1:picTrack2);
				if(IsVisible(spr_id,f.ScrX+17,f.ScrY+14))
					mainTree.AddSprite(DRAW_ORDER_HEX(f.Pos),f.ScrX+17,f.ScrY+14,spr_id,NULL,NULL,NULL,NULL,NULL,NULL);
			}

			// Hex Lines
			if(isShowHex)
			{
				int lt_pos=hTop*wVisible+wRight+VIEW_WIDTH;
				int lb_pos=(hTop+VIEW_HEIGHT-1)*wVisible+wRight+VIEW_WIDTH;
				int rb_pos=(hTop+VIEW_HEIGHT-1)*wVisible+wRight+1;
				int rt_pos=hTop*wVisible+wRight+1;

				int lt_pos2=(hTop+1)*wVisible+wRight+VIEW_WIDTH;
				int lb_pos2=(hTop+VIEW_HEIGHT-1-1)*wVisible+wRight+VIEW_WIDTH;
				int rb_pos2=(hTop+VIEW_HEIGHT-1-1)*wVisible+wRight+1;
				int rt_pos2=(hTop+1)*wVisible+wRight+1;
				bool thru=(vpos==lt_pos || vpos==lb_pos || vpos==rb_pos || vpos==rt_pos ||
					vpos==lt_pos2 || vpos==lb_pos2 || vpos==rb_pos2 || vpos==rt_pos2);

				DWORD spr_id=(thru?hexBlue:hexWhite);
				//DWORD spr_id=(f.ScrollBlock?hexBlue:hexWhite);
				//if(IsVisible(spr_id,f.ScrX+HEX_OX,f.ScrY+HEX_OY))
					mainTree.AddSprite(DRAW_ORDER_HEX(f.Pos),f.ScrX+HEX_OX,f.ScrY+HEX_OY,spr_id,NULL,NULL,NULL,NULL,NULL,NULL);
			}

			// Rain
			if(rainCapacity)
			{
				if(rainCapacity>=Random(0,255) && x>=wRight-1 && x<=(wVisible-wLeft+1) && ty>=hTop-2 && ty<=hVisible)
				{
					int rofx=nx;
					int rofy=ny;
					if(rofx%2) rofx--;
					if(rofy%2) rofy--;

					if(GetField(rofx,rofy).RoofId<2)
					{
						Drop* new_drop=new Drop(picRainDrop,random(10)-random(10),-random(200),0);
						rainData.push_back(new_drop);

						mainTree.AddSprite(DRAW_ORDER_HEX(f.Pos),f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&new_drop->CurSprId,
							&new_drop->OffsX,&new_drop->OffsY,NULL,GetLightHex(nx,ny),NULL);
					}
					else if(!roofSkip || roofSkip!=GetField(rofx,rofy).RoofNum)
					{
						Drop* new_drop=new Drop(picRainDrop,random(10)-random(10),-(100+random(100)),-100);
						rainData.push_back(new_drop);

						mainTree.AddSprite(DRAW_ORDER_HEX(f.Pos),f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&new_drop->CurSprId,
							&new_drop->OffsX,&new_drop->OffsY,NULL,GetLightHex(nx,ny),NULL);
					}
				}
			}

			// Items on hex
			if(!f.Items.empty())
			{
				for(ItemHexVecIt it=f.Items.begin(),end=f.Items.end();it!=end;++it)
				{
					ItemHex* item=*it;

#ifdef FONLINE_CLIENT
					if(item->IsHidden() || item->Proto->TransVal==0xFF) continue;
					if(item->IsScenOrGrid() && !OptShowScen) continue;
					if(item->IsItem() && !OptShowItem) continue;
					if(item->IsWall() && !OptShowWall) continue;
#else // FONLINE_MAPPER
					bool is_fast=fastPids.count(item->GetProtoId())!=0;
					if(item->IsScenOrGrid() && !OptShowScen && !is_fast) continue;
					if(item->IsItem() && !OptShowItem && !is_fast) continue;
					if(item->IsWall() && !OptShowWall && !is_fast) continue;
					if(!OptShowFast && is_fast) continue;
					if(ignorePids.count(item->GetProtoId())) continue;
#endif

					Sprite& spr=mainTree.AddSprite(item->IsFlat()?DRAW_ORDER_ITEM_FLAT(item->IsScenOrGrid()):DRAW_ORDER_ITEM(item->GetPos()),
						f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&item->SprId,&item->ScrX,&item->ScrY,&item->Alpha,
						(item->IsNoLightInfluence() || (item->IsFlat() && item->IsScenOrGrid()))?NULL:GetLightHex(nx,ny),&item->SprDrawValid);
					item->SprDraw=&spr;
					item->SetEffects(&spr);
				}
			}

			// Critters
			CritterCl* cr=f.Crit;
			if(cr && OptShowCrit && cr->Visible)
			{
				Sprite& spr=mainTree.AddSprite(DRAW_ORDER_CRIT(f.Pos),
					f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&cr->SprId,&cr->SprOx,&cr->SprOy,
					&cr->Alpha,GetLightHex(nx,ny),&cr->SprDrawValid);
				cr->SprDraw=&spr;

				cr->SetSprRect();
				if(cr->GetId()==critterContourCrId) spr.Contour=critterContour;
				else if(!cr->IsChosen()) spr.Contour=crittersContour;
				spr.ContourColor=cr->ContourColor;
			}

			// Dead critters
			if(!f.DeadCrits.empty() && OptShowCrit)
			{
				for(CritVecIt it=f.DeadCrits.begin(),end=f.DeadCrits.end();it!=end;++it)
				{
					CritterCl* cr=*it;
					if(!cr->Visible) continue;

					Sprite& spr=mainTree.AddSprite(cr->IsPerk(MODE_NO_FLATTEN)?DRAW_ORDER_CRIT(f.Pos):DRAW_ORDER_CRIT_DEAD(f.Pos),
						f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&cr->SprId,&cr->SprOx,&cr->SprOy,&cr->Alpha,GetLightHex(nx,ny),&cr->SprDrawValid);
					cr->SprDraw=&spr;

					cr->SetSprRect();
				}
			}
		}
		y2+=wVisible;
	}
	mainTree.SortBySurfaces();
	mainTree.SortByMapPos();

#if defined(FONLINE_CLIENT) || defined(FONLINE_MAPPER)
	// Script map draw
	if(MapperFunctions.RenderMap && Script::PrepareContext(MapperFunctions.RenderMap,CALL_FUNC_STR,"Game"))
	{
		SpritesCanDrawMap=true;
		Script::RunPrepared();
		SpritesCanDrawMap=false;
	}
#endif

	screenHexX=rx;
	screenHexY=ry;

#ifdef FONLINE_MAPPER
	if(CurProtoMap)
	{
		CurProtoMap->Header.CenterX=rx;
		CurProtoMap->Header.CenterY=ry;
	}
#endif
}

/************************************************************************/
/* Light                                                                */
/************************************************************************/

#define MAX_LIGHT_VALUE      (10000)
#define MAX_LIGHT_HEX        (200)
#define MAX_LIGHT_ALPHA      (100)
int LightCapacity=0;
int LightMinHx=0;
int LightMaxHx=0;
int LightMinHy=0;
int LightMaxHy=0;
int LightProcentR=0;
int LightProcentG=0;
int LightProcentB=0;

void HexManager::MarkLight(WORD hx, WORD hy, DWORD inten)
{
	int light=inten*MAX_LIGHT_HEX/MAX_LIGHT_VALUE*LightCapacity/100;
	int lr=light*LightProcentR/100;
	int lg=light*LightProcentG/100;
	int lb=light*LightProcentB/100;
	BYTE* p=GetLightHex(hx,hy);
	if(lr>*p) *p=lr;
	if(lg>*(p+1)) *(p+1)=lg;
	if(lb>*(p+2)) *(p+2)=lb;
}

void HexManager::MarkLightEndNeighbor(WORD hx, WORD hy, bool north_south, DWORD inten)
{
	Field& f=GetField(hx,hy);
	if(f.IsWall)
	{
		int lt=f.LightType;
		if((north_south && (lt==LIGHT_NORTH_SOUTH || lt==LIGHT_NORTH || lt==LIGHT_WEST))
		|| (!north_south && (lt==LIGHT_EAST_WEST || lt==LIGHT_EAST))
		|| lt==LIGHT_SOUTH)
		{
			BYTE* p=GetLightHex(hx,hy);
			int light_full=inten*MAX_LIGHT_HEX/MAX_LIGHT_VALUE*LightCapacity/100;
			int light_self=(inten/2)*MAX_LIGHT_HEX/MAX_LIGHT_VALUE*LightCapacity/100;
			int lr_full=light_full*LightProcentR/100;
			int lg_full=light_full*LightProcentG/100;
			int lb_full=light_full*LightProcentB/100;
			int lr_self=int(*p)+light_self*LightProcentR/100;
			int lg_self=int(*(p+1))+light_self*LightProcentG/100;
			int lb_self=int(*(p+2))+light_self*LightProcentB/100;
			if(lr_self>lr_full) lr_self=lr_full;
			if(lg_self>lg_full) lg_self=lg_full;
			if(lb_self>lb_full) lb_self=lb_full;
			if(lr_self>*p) *p=lr_self;
			if(lg_self>*(p+1)) *(p+1)=lg_self;
			if(lb_self>*(p+2)) *(p+2)=lb_self;
		}
	}
}

void HexManager::MarkLightEnd(WORD from_hx, WORD from_hy, WORD to_hx, WORD to_hy, DWORD inten)
{
	bool is_wall=false;
	bool north_south=false;
	Field& f=GetField(to_hx,to_hy);
	if(f.IsWall)
	{
		is_wall=true;
		if(f.LightType==LIGHT_NORTH_SOUTH || f.LightType==LIGHT_NORTH || f.LightType==LIGHT_WEST) north_south=true;
	}

	int dir=GetFarDir(from_hx,from_hy,to_hx,to_hy);
	if(dir==0 || (north_south && dir==1) || (!north_south && (dir==4 || dir==5)))
	{
		MarkLight(to_hx,to_hy,inten);
		if(is_wall)
		{
			if(north_south)
			{
				if(to_hy>0) MarkLightEndNeighbor(to_hx,to_hy-1,true,inten);
				if(to_hy<maxHexY-1) MarkLightEndNeighbor(to_hx,to_hy+1,true,inten);
			}
			else
			{
				if(to_hx>0)
				{
					MarkLightEndNeighbor(to_hx-1,to_hy,false,inten);
					if(to_hy>0) MarkLightEndNeighbor(to_hx-1,to_hy-1,false,inten);
					if(to_hy<maxHexY-1) MarkLightEndNeighbor(to_hx-1,to_hy+1,false,inten);
				}
				if(to_hx<maxHexX-1)
				{
					MarkLightEndNeighbor(to_hx+1,to_hy,false,inten);
					if(to_hy>0) MarkLightEndNeighbor(to_hx+1,to_hy-1,false,inten);
					if(to_hy<maxHexY-1) MarkLightEndNeighbor(to_hx+1,to_hy+1,false,inten);
				}
			}
		}
	}
}

void HexManager::MarkLightStep(WORD from_hx, WORD from_hy, WORD to_hx, WORD to_hy, DWORD inten)
{
	Field& f=GetField(to_hx,to_hy);
	if(f.IsWallTransp)
	{
		bool north_south=(f.LightType==LIGHT_NORTH_SOUTH || f.LightType==LIGHT_NORTH || f.LightType==LIGHT_WEST);
		int dir=GetFarDir(from_hx,from_hy,to_hx,to_hy);
		if(dir==0 || (north_south && dir==1) || (!north_south && (dir==4 || dir==5))) MarkLight(to_hx,to_hy,inten);
	}
	else
	{
		MarkLight(to_hx,to_hy,inten);
	}
}

void HexManager::TraceLight(WORD from_hx, WORD from_hy, WORD& hx, WORD& hy, int dist, DWORD inten)
{
	float base_sx,base_sy;
	GetStepsXY(base_sx,base_sy,from_hx,from_hy,hx,hy);
	float sx1f=base_sx;
	float sy1f=base_sy;	
	float curx1f=(float)from_hx;
	float cury1f=(float)from_hy;
	int curx1i=from_hx;
	int cury1i=from_hy;
	int old_curx1i=curx1i;
	int old_cury1i=cury1i;
	bool right_barr=false;
	bool left_barr=false;
	DWORD inten_sub=inten/dist;

	for(;;)
	{
		inten-=inten_sub;
		curx1f+=sx1f;
		cury1f+=sy1f;
		old_curx1i=curx1i;
		old_cury1i=cury1i;

		// Casts
		curx1i=(int)curx1f;
		if(curx1f-(float)curx1i>=0.5f) curx1i++;
		cury1i=(int)cury1f;
		if(cury1f-(float)cury1i>=0.5f) cury1i++;
		bool can_mark=(curx1i>=LightMinHx && curx1i<=LightMaxHx && cury1i>=LightMinHy && cury1i<=LightMaxHy);

		// Left&Right trace
		int ox=0;
		int oy=0;

		if(old_curx1i%2)
		{
			if(old_curx1i+1==curx1i && old_cury1i+1==cury1i) {ox= 1; oy= 1;}
			if(old_curx1i-1==curx1i && old_cury1i+1==cury1i) {ox=-1; oy= 1;}
		}
		else
		{
			if(old_curx1i-1==curx1i && old_cury1i-1==cury1i) {ox=-1; oy=-1;}
			if(old_curx1i+1==curx1i && old_cury1i-1==cury1i) {ox= 1; oy=-1;}
		}

		if(ox)
		{
			// Left side
			ox=old_curx1i+ox;
			if(ox<0 || ox>=maxHexX || GetField(ox,old_cury1i).IsNoLight)
			{
				hx=(ox<0 || ox>=maxHexX?old_curx1i:ox);
				hy=old_cury1i;
				if(can_mark) MarkLightEnd(old_curx1i,old_cury1i,hx,hy,inten);
				break;
			}
			if(can_mark) MarkLightStep(old_curx1i,old_cury1i,ox,old_cury1i,inten);

			// Right side
			oy=old_cury1i+oy;
			if(oy<0 || oy>=maxHexY || GetField(old_curx1i,oy).IsNoLight)
			{
				hx=old_curx1i;
				hy=(oy<0 || oy>=maxHexY?old_cury1i:oy);
				if(can_mark) MarkLightEnd(old_curx1i,old_cury1i,hx,hy,inten);
				break;
			}
			if(can_mark) MarkLightStep(old_curx1i,old_cury1i,old_curx1i,oy,inten);
		}

		// Main trace
		if(curx1i<0 || curx1i>=maxHexX || cury1i<0 || cury1i>=maxHexY || GetField(curx1i,cury1i).IsNoLight)
		{
			hx=(curx1i<0 || curx1i>=maxHexX?old_curx1i:curx1i);
			hy=(cury1i<0 || cury1i>=maxHexY?old_cury1i:cury1i);
			if(can_mark) MarkLightEnd(old_curx1i,old_cury1i,hx,hy,inten);
			break;
		}
		if(can_mark) MarkLightEnd(old_curx1i,old_cury1i,curx1i,cury1i,inten);
		if(curx1i==hx && cury1i==hy) break;
	}
}

void HexManager::ParseLightTriangleFan(LightSource& ls)
{
	WORD hx=ls.HexX;
	WORD hy=ls.HexY;
	BYTE dir_off=ls.Flags&63;
	// Distantion
	int dist=ls.Radius;
	if(dist<1) dist=1;
	// Intensity
	int inten=ABS(ls.Intensity);
	if(inten>100) inten=50;
	inten*=100;
	if(ls.Flags&0x40) GetColorDay(GetMapDayTime(),GetMapDayColor(),GetDayTime(),&LightCapacity);
	else if(ls.Intensity>=0) GetColorDay(GetMapDayTime(),GetMapDayColor(),GetMapTime(),&LightCapacity);
	else LightCapacity=100;
	if(ls.Flags&0x80) LightCapacity=100-LightCapacity;
	// Color
	DWORD color=ls.ColorRGB;
	if(color==0) color=0xFFFFFF;
	int alpha=MAX_LIGHT_ALPHA*LightCapacity/100*inten/MAX_LIGHT_VALUE;
	color=D3DCOLOR_ARGB(alpha,(color>>16)&0xFF,(color>>8)&0xFF,color&0xFF);
	LightProcentR=((color>>16)&0xFF)*100/0xFF;
	LightProcentG=((color>>8)&0xFF)*100/0xFF;
	LightProcentB=(color&0xFF)*100/0xFF;

	// Begin
	MarkLight(hx,hy,inten);
	int base_x,base_y;
	GetHexCurrentPosition(hx,hy,base_x,base_y);
	base_x+=16;
	base_y+=6;

	lightPointsCount++;
	if(lightPoints.size()<lightPointsCount) lightPoints.push_back(PointVec());
	PointVec& points=lightPoints[lightPointsCount-1];
	points.clear();
	points.reserve(3+dist*6);
	points.push_back(PrepPoint(base_x,base_y,color,(short*)&CmnScrOx,(short*)&CmnScrOy)); // Center of light
	color=D3DCOLOR_ARGB(0,(color>>16)&0xFF,(color>>8)&0xFF,color&0xFF);

	const int dirs[6]={2,3,4,5,0,1};
	int hx_far=hx,hy_far=hy;
	bool seek_start=true;
	WORD last_hx=-1,last_hy=-1;

	for(int i=0;i<6;i++)
	{
		for(int j=0;j<dist;j++)
		{
			if(seek_start)
			{
				// Move to start position
				for(int l=0;l<dist;l++) MoveHexByDirUnsafe(hx_far,hy_far,0);
				seek_start=false;
				j=-1;
			}
			else
			{
				// Move to next hex
				MoveHexByDirUnsafe(hx_far,hy_far,dirs[i]);
			}

			WORD hx_=CLAMP(hx_far,0,maxHexX-1);
			WORD hy_=CLAMP(hy_far,0,maxHexY-1);
			if(j>=0 && (dir_off>>i)&1)
			{
				hx_=hx;
				hy_=hy;
			}
			else
			{
				TraceLight(hx,hy,hx_,hy_,dist,inten);
			}

			if(hx_!=last_hx || hy_!=last_hy)
			{
				if((int)hx_!=hx_far || (int)hy_!=hy_far)
				{
					int a=alpha-DistGame(hx,hy,hx_,hy_)*alpha/dist;
					a=CLAMP(a,0,alpha);
					color=D3DCOLOR_ARGB(a,(color>>16)&0xFF,(color>>8)&0xFF,color&0xFF);
				}
				else color=D3DCOLOR_ARGB(0,(color>>16)&0xFF,(color>>8)&0xFF,color&0xFF);
				int x,y;
				GetHexOffset(hx,hy,hx_,hy_,x,y);
				points.push_back(PrepPoint(base_x+x,base_y+y,color,(short*)&CmnScrOx,(short*)&CmnScrOy));
				last_hx=hx_;
				last_hy=hy_;
			}
		}
	}

	for(int i=1,j=points.size();i<j;i++)
	{
		PrepPoint& cur=points[i];
		PrepPoint& next=points[i>=points.size()-1?1:i+1];
		if(DistSqrt(cur.PointX,cur.PointY,next.PointX,next.PointY)>32)
		{
			bool dist_comp=(DistSqrt(base_x,base_y,cur.PointX,cur.PointY)>DistSqrt(base_x,base_y,next.PointX,next.PointY));
			lightSoftPoints.push_back(PrepPoint(next.PointX,next.PointY,next.PointColor,(short*)&CmnScrOx,(short*)&CmnScrOy));
			lightSoftPoints.push_back(PrepPoint(cur.PointX,cur.PointY,cur.PointColor,(short*)&CmnScrOx,(short*)&CmnScrOy));
			float x=(dist_comp?next.PointX-cur.PointX:cur.PointX-next.PointX);
			float y=(dist_comp?next.PointY-cur.PointY:cur.PointY-next.PointY);
			ChangeStepsXY(x,y,dist_comp?-2.5f:2.5f);
			if(dist_comp) lightSoftPoints.push_back(PrepPoint(cur.PointX+int(x),cur.PointY+int(y),cur.PointColor,(short*)&CmnScrOx,(short*)&CmnScrOy));
			else lightSoftPoints.push_back(PrepPoint(next.PointX+int(x),next.PointY+int(y),next.PointColor,(short*)&CmnScrOx,(short*)&CmnScrOy));
		}
	}
}

void HexManager::RebuildLight()
{
	lightPointsCount=0;
	lightSoftPoints.clear();
	if(!viewField) return;

	ClearHexLight();
	CollectLightSources();

	LightMinHx=viewField[0].HexX;
	LightMaxHx=viewField[hVisible*wVisible-1].HexX;
	LightMinHy=viewField[wVisible-1].HexY;
	LightMaxHy=viewField[hVisible*wVisible-wVisible].HexY;

	for(LightSourceVecIt it=lightSources.begin(),end=lightSources.end();it!=end;++it)
	{
		LightSource& ls=(*it);
	//	if( (int)ls.HexX<LightMinHx-(int)ls.Radius || (int)ls.HexX>LightMaxHx+(int)ls.Radius ||
	//		(int)ls.HexY<LightMinHy-(int)ls.Radius || (int)ls.HexY>LightMaxHy+(int)ls.Radius) continue;
		ParseLightTriangleFan(ls);
	}

#ifdef FONLINE_CLIENT
	// Chosen light
	CritterCl* chosen=GetChosen();
	if(chosen && chosen->Visible) ParseLightTriangleFan(LightSource(chosen->GetHexX(),chosen->GetHexY(),0,4,MAX_LIGHT_VALUE/4,0));

	/*for(CritMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		CritterCl* cr=(*it).second;
		if(cr->IsChosen())
		{
			if(cr->Visible) ParseLightTriangleFan(LightSource(cr->GetHexX(),cr->GetHexY(),0,4,MAX_LIGHT_VALUE/4,0));
			break;
		}
	}*/
#endif // FONLINE_CLIENT
}

void HexManager::CollectLightSources()
{
	lightSources.clear();

#ifdef FONLINE_MAPPER
	if(!CurProtoMap) return;

	for(int i=0,j=CurProtoMap->MObjects.size();i<j;i++)
	{
		MapObject* o=CurProtoMap->MObjects[i];
		if(o->LightIntensity && !(o->MapObjType==MAP_OBJECT_ITEM && o->MItem.InContainer))
			lightSources.push_back(LightSource(o->MapX,o->MapY,o->LightRGB,o->LightRadius,o->LightIntensity,o->LightDirOff|((o->LightDay&3)<<6)));
	}
#else
	lightSources=lightSourcesScen;
	for(ItemHexVecIt it=hexItems.begin(),end=hexItems.end();it!=end;++it)
	{
		ItemHex* item=(*it);
		if(item->IsItem() && item->IsLight()) lightSources.push_back(LightSource(item->GetHexX(),item->GetHexY(),item->LightGetColor(),item->LightGetRadius(),item->LightGetIntensity(),item->LightGetFlags()));
	}
#endif
}

/************************************************************************/
/* Tiles                                                                */
/************************************************************************/

bool HexManager::CheckTilesBorder(DWORD spr_id, bool is_roof)
{
	int ox=(is_roof?ROOF_OX:TILE_OX);
	int oy=(is_roof?ROOF_OY:TILE_OY);
	return ProcessHexBorders(spr_id,ox,oy);
}

void HexManager::RebuildTiles()
{
	Sprites ttree;

	if(!OptShowTile)
	{
		tileCntPrep=0;
		sprMngr->PrepareBuffer(ttree,tileVB,tilePrepSurf,false,0);
		return;
	}

	ttree.Resize(VIEW_WIDTH*VIEW_HEIGHT);

	int vpos;
	int y2=0;
	for(int ty=0;ty<hVisible;ty++)
	{
		for(int x=0;x<wVisible;x++)
		{
			vpos=y2+x;
			if(vpos<0 || vpos>=wVisible*hVisible) continue;
			int hx=viewField[vpos].HexX;
			int hy=viewField[vpos].HexY;

			if(hx%2 || hy%2) continue;
			if(hy<0 || hx<0 || hy>=maxHexY || hx>=maxHexX) continue;

			Field& f=GetField(hx,hy);

#ifdef FONLINE_MAPPER
			if(f.SelTile)
			{
				if(IsVisible(f.SelTile,f.ScrX+TILE_OX,f.ScrY+TILE_OY))
					ttree.AddSprite(0,f.ScrX+TILE_OX,f.ScrY+TILE_OY,f.SelTile,NULL,NULL,NULL,(BYTE*)&SELECT_ALPHA,NULL,NULL);
				continue;
			}
#endif

			if(f.TileId && IsVisible(f.TileId,f.ScrX+TILE_OX,f.ScrY+TILE_OY))
				ttree.AddSprite(0,f.ScrX+TILE_OX,f.ScrY+TILE_OY,f.TileId,NULL,NULL,NULL,NULL,NULL,NULL);
		}
		y2+=wVisible;
	}

	tileCntPrep=ttree.Size();
	sprMngr->PrepareBuffer(ttree,tileVB,tilePrepSurf,true,TILE_ALPHA);
	ttree.Clear();
}

void HexManager::RebuildRoof()
{
	roofTree.Unvalidate();
	if(!OptShowRoof) return;

	int vpos;
	int y2=0;
	for(int ty=0;ty<hVisible;ty++)
	{
		for(int x=0;x<wVisible;x++)
		{
			vpos=y2+x;
			int hx=viewField[vpos].HexX;
			int hy=viewField[vpos].HexY;

			if(hx%2 || hy%2) continue;
			if(hy<0 || hx<0 || hy>=maxHexY || hx>=maxHexX) continue;

			Field& f=GetField(hx,hy);

#ifdef FONLINE_MAPPER
				if(f.SelRoof)
				{
					if(IsVisible(f.SelRoof,f.ScrX+ROOF_OX,f.ScrY+ROOF_OY))
					{
						Sprite& spr=roofTree.AddSprite(0,f.ScrX+ROOF_OX,f.ScrY+ROOF_OY,f.SelRoof,NULL,NULL,NULL,(BYTE*)&SELECT_ALPHA,NULL,NULL);
						spr.Egg=Sprite::EggAlways;
					}
					continue;
				}
#endif

			if(f.RoofId && (!roofSkip || roofSkip!=f.RoofNum) && IsVisible(f.RoofId,f.ScrX+ROOF_OX,f.ScrY+ROOF_OY))
			{
				Sprite& spr=roofTree.AddSprite(0,f.ScrX+ROOF_OX,f.ScrY+ROOF_OY,f.RoofId,NULL,NULL,NULL,&ROOF_ALPHA,NULL,NULL);
				spr.Egg=Sprite::EggAlways;
			}
		}
		y2+=wVisible;
	}
}

void HexManager::SetSkipRoof(int hx, int hy)
{
	if(hx%2) hx--;
	if(hy%2) hy--;

	if(roofSkip!=GetField(hx,hy).RoofNum)
	{
		roofSkip=GetField(hx,hy).RoofNum;
		if(rainCapacity) RefreshMap();
		else RebuildRoof();
	}
}

void HexManager::MarkRoofNum(int hx, int hy, int num)
{
	if(hx<0 || hx>=maxHexX || hy<0 || hy>=maxHexY) return;
	if(!GetField(hx,hy).RoofId) return;
	if(GetField(hx,hy).RoofNum) return;

	GetField(hx,hy).RoofNum=num;

	MarkRoofNum(hx+2,hy,num);
	MarkRoofNum(hx-2,hy,num);
	MarkRoofNum(hx,hy+2,num);
	MarkRoofNum(hx,hy-2,num);
}

bool HexManager::IsVisible(DWORD spr_id, int ox, int oy)
{
	if(!spr_id) return false;
	SpriteInfo* si=sprMngr->GetSpriteInfo(spr_id);
	if(!si) return false;
	if(si->Anim3d) return true;

	int top=oy+si->OffsY-si->Height-SCROLL_OY;
	int bottom=oy+si->OffsY+SCROLL_OY;
	int left=ox+si->OffsX-si->Width/2-SCROLL_OX;
	int right=ox+si->OffsX+si->Width/2+SCROLL_OX;
	return !(top>MODE_HEIGHT*ZOOM || bottom<0 || left>MODE_WIDTH*ZOOM || right<0);
}

bool HexManager::ProcessHexBorders(DWORD spr_id, int ox, int oy)
{
	SpriteInfo* si=sprMngr->GetSpriteInfo(spr_id);
	if(!si) return false;

	int top=si->OffsY+oy-hTop*12+SCROLL_OY;
	if(top<0) top=0;
	int bottom=si->Height-si->OffsY-oy-hBottom*12+SCROLL_OY;
	if(bottom<0) bottom=0;
	int left=si->Width/2+si->OffsX+ox-wLeft*32+SCROLL_OX;
	if(left<0) left=0;
	int right=si->Width/2-si->OffsX-ox-wRight*32+SCROLL_OX;
	if(right<0) right=0;

	if(top || bottom || left || right)
	{
		// Resize
		hTop+=top/12+(top%12?1:0);
		hBottom+=bottom/12+(bottom%12?1:0);
		wLeft+=left/32+(left%32?1:0);
		wRight+=right/32+(right%32?1:0);
		return true;
	}
	return false;
}

void HexManager::SwitchShowHex()
{
	isShowHex=isShowHex?0:1;
	RefreshMap();
}

void HexManager::SwitchShowRain()
{
	rainCapacity=(rainCapacity?0:255);
	RefreshMap();
}

void HexManager::SetWeather(int time, BYTE rain)
{
	curMapTime=time;
	rainCapacity=rain;
}

bool HexManager::ResizeField(WORD w, WORD h)
{
	maxHexX=w;
	maxHexY=h;
	SAFEDELA(hexField);
	SAFEDELA(hexToDraw);
	SAFEDELA(hexTrack);
	SAFEDELA(hexLight);
	if(!w || !h) return true;

	hexField=new Field[w*h];
	if(!hexField) return false;
	hexToDraw=new bool[w*h];
	if(!hexToDraw) return false;
	ZeroMemory(hexToDraw,w*h*sizeof(bool));
	hexTrack=new char[w*h];
	if(!hexTrack) return false;
	ZeroMemory(hexTrack,w*h*sizeof(char));
	hexLight=new BYTE[w*h*3];
	if(!hexLight) return false;
	ZeroMemory(hexLight,w*h*3*sizeof(BYTE));

	for(int hx=0;hx<w;hx++)
		for(int hy=0;hy<h;hy++) GetField(hx,hy).Pos=HEX_POS(hx,hy);
	return true;
}

void HexManager::SwitchShowTrack()
{
	isShowTrack=!isShowTrack;
	if(!isShowTrack) ClearHexTrack();
	RefreshMap();
}

void HexManager::InitView(int cx, int cy)
{
	// Get center offset
	int hw=VIEW_WIDTH/2+wRight;
	int hv=VIEW_HEIGHT/2+hTop;
	int vw=hv/2+hv%2+1;
	int vh=hv-vw/2-1;
 	for(int i=0;i<hw;i++)
 	{
 		if(vw%2) vh--;
 		vw++;
 	}

	// Substract offset
	cx-=ABS(vw);
	cy-=ABS(vh);

	int x;
	int xa=-(wRight*32);
	int xb=-16-(wRight*32);
	int y=-12*hTop;
	int y2=0;
	int vpos;
	int hx,hy;

	for(int j=0;j<hVisible;j++)
	{
		hx=cx+j/2+j%2;
		hy=cy+(j-(hx-cx-(cx%2?1:0))/2);
		x=(j%2?xa:xb);

		for(int i=0;i<wVisible;i++)
		{
			vpos=y2+i;
			viewField[vpos].ScrX=MODE_WIDTH*ZOOM-x;
			viewField[vpos].ScrY=y;
		//	viewField[vpos].HexX=(hx>=0 && hx<maxHexX?hx:0xFFFF);
		//	viewField[vpos].HexY=(hy>=0 && hy<maxHexY?hy:0xFFFF);
			viewField[vpos].HexX=hx;
			viewField[vpos].HexY=hy;

			if(hx%2) hy--;
			hx++;
			x+=32;
		}
		y+=12;
		y2+=wVisible;
	}
}

void HexManager::ChangeZoom(float offs)
{
	if(!IsMapLoaded()) return;
	if(OptZoom+offs>10.0f) return;
	if(OptZoom+offs<0.7f) return;

#ifdef FONLINE_CLIENT
	// Check screen blockers
	if(offs>0)
	{
		for(int x=-3;x<4;x++)
		{
			for(int y=-3;y<4;y++)
			{
				if(!x && !y) continue;
				if(ScrollCheck(x,y)) return;
			}
		}
	}
#endif

	OptZoom+=offs;

	hVisible=VIEW_HEIGHT+hTop+hBottom;
	wVisible=VIEW_WIDTH+wLeft+wRight;

	delete[] viewField;
	viewField=new ViewField[hVisible*wVisible];

	RefreshMap();
}

void HexManager::GetHexOffset(int from_hx, int from_hy, int to_hx, int to_hy, int& x, int& y)
{
	int dx=to_hx-from_hx;
	int dy=to_hy-from_hy;
	int xc=(dx+(min(from_hx,to_hx)%2?(dx<0?-1:1):0))/2;
	x=dx*32;
	x-=xc*16;
	x-=dy*16;
	x=-x;
	y=dy*12;
	y+=xc*12;
}

void HexManager::GetHexCurrentPosition(WORD hx, WORD hy, int& x, int& y)
{
	ViewField& center_hex=viewField[hVisible/2*wVisible+wVisible/2];
	int center_hx=center_hex.HexX;
	int center_hy=center_hex.HexY;

	GetHexOffset(center_hx,center_hy,hx,hy,x,y);
	x+=center_hex.ScrX;
	y+=center_hex.ScrY;

//	x+=center_hex.ScrX+CmnScrOx;
//	y+=center_hex.ScrY+CmnScrOy;
//	x/=ZOOM;
//	y/=ZOOM;
}

void HexManager::DrawMap()
{
	// Tiles
	if(OptShowTile) sprMngr->DrawPrepared(tileVB,tilePrepSurf,tileCntPrep);
	sprMngr->DrawTreeCntr(mainTree,false,false,0,0);

	// Light
	for(int i=0;i<lightPointsCount;i++) sprMngr->DrawPoints(lightPoints[i],D3DPT_TRIANGLEFAN,true);
	sprMngr->DrawPoints(lightSoftPoints,D3DPT_TRIANGLELIST,true);

	// Cursor Pre
	DrawCursor(cursorPrePic);

	// Items
	sprMngr->DrawTreeCntr(mainTree,true,true,1,-1);

	// Roof
	if(OptShowRoof)
	{
		sprMngr->DrawTreeCntr(roofTree,false,true,0,-1);
		if(rainCapacity) sprMngr->DrawTreeCntr(roofRainTree,false,false,0,-1);
	}

	// Contours
	sprMngr->DrawContours();

	// Cursor Post
	DrawCursor(cursorPostPic);
	if(drawCursorX<0) DrawCursor(cursorXPic);
	else if(drawCursorX>0) DrawCursor(Str::Format("%u",drawCursorX));

	sprMngr->Flush();
}

bool HexManager::Scroll()
{
	if(!IsMapLoaded()) return false;

	static DWORD last_call=Timer::AccurateTick();
	if(Timer::AccurateTick()-last_call<OptScrollDelay) return false;
	else last_call=Timer::AccurateTick();

	bool is_scroll=(CmnDiMleft || CmnDiLeft || CmnDiMright || CmnDiRight || CmnDiMup || CmnDiUp || CmnDiMdown || CmnDiDown);
	int scr_ox=CmnScrOx;
	int scr_oy=CmnScrOy;

	if(is_scroll && AutoScroll.CanStop) AutoScroll.Active=false;

	// Check critter scroll lock
	if(AutoScroll.LockedCritter && !is_scroll)
	{
		CritterCl* cr=GetCritter(AutoScroll.LockedCritter);
		if(cr && (cr->GetHexX()!=screenHexX || cr->GetHexY()!=screenHexY)) ScrollToHex(cr->GetHexX(),cr->GetHexY(),0.02,true);
		//if(cr && DistSqrt(cr->GetHexX(),cr->GetHexY(),screenHexX,screenHexY)>4) ScrollToHex(cr->GetHexX(),cr->GetHexY(),0.5,true);
	}

	if(AutoScroll.Active)
	{
		AutoScroll.OffsXStep+=AutoScroll.OffsX*AutoScroll.Speed;
		AutoScroll.OffsYStep+=AutoScroll.OffsY*AutoScroll.Speed;
		int xscroll=(int)AutoScroll.OffsXStep;
		int yscroll=(int)AutoScroll.OffsYStep;
		if(xscroll>SCROLL_OX)
		{
			xscroll=SCROLL_OX;
			AutoScroll.OffsXStep=(double)SCROLL_OX;
		}
		if(xscroll<-SCROLL_OX)
		{
			xscroll=-SCROLL_OX;
			AutoScroll.OffsXStep=-(double)SCROLL_OX;
		}
		if(yscroll>SCROLL_OY)
		{
			yscroll=SCROLL_OY;
			AutoScroll.OffsYStep=(double)SCROLL_OY;
		}
		if(yscroll<-SCROLL_OY)
		{
			yscroll=-SCROLL_OY;
			AutoScroll.OffsYStep=-(double)SCROLL_OY;
		}

		AutoScroll.OffsX-=xscroll;
		AutoScroll.OffsY-=yscroll;
		AutoScroll.OffsXStep-=xscroll;
		AutoScroll.OffsYStep-=yscroll;
		if(!xscroll && !yscroll) return false;
		if(!DistSqrt(0,0,AutoScroll.OffsX,AutoScroll.OffsY)) AutoScroll.Active=false;

		scr_ox+=xscroll;
		scr_oy+=yscroll;
	}
	else
	{
		if(!is_scroll) return false;

		int xscroll=0;
		int yscroll=0;

		if(CmnDiMleft || CmnDiLeft) xscroll+=1;
		if(CmnDiMright || CmnDiRight) xscroll-=1;
		if(CmnDiMup || CmnDiUp) yscroll+=1;
		if(CmnDiMdown || CmnDiDown) yscroll-=1;
		if(!xscroll && !yscroll) return false;

		scr_ox+=xscroll*OptScrollStep;
		scr_oy+=yscroll*(OptScrollStep*75/100);
	}

	if(OptScrollCheck)
	{
		int xmod=0;
		int ymod=0;
		if(scr_ox-CmnScrOx>0) xmod=1;
		if(scr_ox-CmnScrOx<0) xmod=-1;
		if(scr_oy-CmnScrOy>0) ymod=-1;
		if(scr_oy-CmnScrOy<0) ymod=1;
		if((xmod || ymod) && ScrollCheck(xmod,ymod))
		{
			if(xmod && ymod && !ScrollCheck(0,ymod)) scr_ox=0;
			else if(xmod && ymod && !ScrollCheck(xmod,0)) scr_oy=0;
			else
			{
				if(xmod) scr_ox=0;
				if(ymod) scr_oy=0;
			}
		}
	}

	int xmod=0;
	int ymod=0;
	if(scr_ox>=SCROLL_OX)
	{
		xmod=1;
		scr_ox-=SCROLL_OX;
		if(scr_ox>SCROLL_OX) scr_ox=SCROLL_OX;
	}
	else if(scr_ox<=-SCROLL_OX)
	{
		xmod=-1;
		scr_ox+=SCROLL_OX;
		if(scr_ox<-SCROLL_OX) scr_ox=-SCROLL_OX;
	}
	if(scr_oy>=SCROLL_OY)
	{
		ymod=-2;
		scr_oy-=SCROLL_OY;
		if(scr_oy>SCROLL_OY) scr_oy=SCROLL_OY;
	}
	else if(scr_oy<=-SCROLL_OY)
	{
		ymod=2;
		scr_oy+=SCROLL_OY;
		if(scr_oy<-SCROLL_OY) scr_oy=-SCROLL_OY;
	}

	CmnScrOx=scr_ox;
	CmnScrOy=scr_oy;

	if(xmod || ymod)
	{
		int vpos1=5*wVisible+4;
		int vpos2=(5+ymod)*wVisible+4+xmod;
		int hx=screenHexX+(viewField[vpos2].HexX-viewField[vpos1].HexX);
		int hy=screenHexY+(viewField[vpos2].HexY-viewField[vpos1].HexY);
		RebuildMap(hx,hy);

		if(OptScrollCheck)
		{
			int old_ox=CmnScrOx;
			int old_oy=CmnScrOy;
			if(CmnScrOx>0 && ScrollCheck(1,0)) CmnScrOx=0;
			else if(CmnScrOx<0 && ScrollCheck(-1,0)) CmnScrOx=0;
			if(CmnScrOy>0 && ScrollCheck(0,-1)) CmnScrOy=0;
			else if(CmnScrOy<0 && ScrollCheck(0,1)) CmnScrOy=0;
			if(CmnScrOx!=old_ox || CmnScrOy!=old_oy) RebuildTiles();
		}
	}
	else
	{
		RebuildTiles();
	}
	
	return true;
}

bool HexManager::ScrollCheckPos(int (&positions)[4], int dir1, int dir2)
{
	int max_pos=wVisible*hVisible;
	for(int i=0;i<4;i++)
	{
		int pos=positions[i];
		if(pos<0 || pos>=max_pos) return true;

		WORD hx=viewField[pos].HexX;
		WORD hy=viewField[pos].HexY;
		if(hx>=maxHexX || hy>=maxHexY) return true;

		MoveHexByDir(hx,hy,dir1,maxHexX,maxHexY);
		if(GetField(hx,hy).ScrollBlock) return true;
		if(dir2>=0)
		{
			MoveHexByDir(hx,hy,dir2,maxHexX,maxHexY);
			if(GetField(hx,hy).ScrollBlock) return true;
		}
	}
	return false;
}

bool HexManager::ScrollCheck(int xmod, int ymod)
{
	int positions_left[4]={
		hTop*wVisible+wRight+VIEW_WIDTH, // Left top
		(hTop+VIEW_HEIGHT-1)*wVisible+wRight+VIEW_WIDTH, // Left bottom
		(hTop+1)*wVisible+wRight+VIEW_WIDTH, // Left top 2
		(hTop+VIEW_HEIGHT-1-1)*wVisible+wRight+VIEW_WIDTH, // Left bottom 2
	};
	int positions_right[4]={
		(hTop+VIEW_HEIGHT-1)*wVisible+wRight+1, // Right bottom
		hTop*wVisible+wRight+1, // Right top
		(hTop+VIEW_HEIGHT-1-1)*wVisible+wRight+1, // Right bottom 2
		(hTop+1)*wVisible+wRight+1, // Right top 2
	};

	// Up
	if(ymod<0 && (ScrollCheckPos(positions_left,0,5) || ScrollCheckPos(positions_right,5,0))) return true;
	// Down
	else if(ymod>0 && (ScrollCheckPos(positions_left,2,3) || ScrollCheckPos(positions_right,3,2))) return true;
	// Left
	if(xmod>0 && (ScrollCheckPos(positions_left,4,-1) || ScrollCheckPos(positions_right,4,-1))) return true;
	// Right
	else if(xmod<0 && (ScrollCheckPos(positions_right,1,-1) || ScrollCheckPos(positions_left,1,-1))) return true;
	return false;
}

void HexManager::ScrollToHex(int hx, int hy, double speed, bool can_stop)
{
	if(!IsMapLoaded()) return;

	int sx,sy;
	GetScreenHexes(sx,sy);
	int x,y;
	GetHexOffset(sx,sy,hx,hy,x,y);
	AutoScroll.Active=true;
	AutoScroll.CanStop=can_stop;
	AutoScroll.OffsX=-x;
	AutoScroll.OffsY=-y;
	AutoScroll.OffsXStep=0.0;
	AutoScroll.OffsYStep=0.0;
	AutoScroll.Speed=speed;
}

void HexManager::PreRestore()
{
	SAFEREL(tileVB);
}

void HexManager::PostRestore()
{
	RefreshMap();
}

void HexManager::SetCrit(CritterCl* cr)
{
	if(!IsMapLoaded()) return;

	WORD hx=cr->GetHexX();
	WORD hy=cr->GetHexY();
	Field& f=GetField(hx,hy);

	if(cr->IsDead())
	{
		if(std::find(f.DeadCrits.begin(),f.DeadCrits.end(),cr)==f.DeadCrits.end()) f.DeadCrits.push_back(cr);
	}
	else
	{
		if(f.Crit && f.Crit!=cr)
		{
			WriteLog(__FUNCTION__" - Hex<%u><%u> busy, critter old<%u>, new<%u>.\n",hx,hy,f.Crit->GetId(),cr->GetId());
			return;
		}

		f.Crit=cr;
	}

	if(GetHexToDraw(hx,hy) && cr->Visible)
	{
		Sprite& spr=mainTree.InsertSprite(cr->IsDead() && !cr->IsPerk(MODE_NO_FLATTEN)?DRAW_ORDER_CRIT_DEAD(f.Pos):DRAW_ORDER_CRIT(f.Pos),
			f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&cr->SprId,&cr->SprOx,&cr->SprOy,&cr->Alpha,GetLightHex(hx,hy),&cr->SprDrawValid);
		cr->SprDraw=&spr;

		cr->SetSprRect();
		cr->FixLastHexes();
		if(cr->GetId()==critterContourCrId) spr.Contour=critterContour;
		else if(!cr->IsDead() && !cr->IsChosen()) spr.Contour=crittersContour;
		spr.ContourColor=cr->ContourColor;
	}

	f.ProcessCache();
}

void HexManager::RemoveCrit(CritterCl* cr)
{
	if(!IsMapLoaded()) return;

	Field& f=GetField(cr->GetHexX(),cr->GetHexY());
	if(f.Crit==cr)
	{
		f.Crit=NULL;
	}
	else
	{
		CritVecIt it=std::find(f.DeadCrits.begin(),f.DeadCrits.end(),cr);
		if(it!=f.DeadCrits.end()) f.DeadCrits.erase(it);
	}

	if(cr->SprDrawValid) cr->SprDraw->Unvalidate();
	f.ProcessCache();
}

void HexManager::AddCrit(CritterCl* cr)
{
	if(allCritters.count(cr->GetId())) return;
	allCritters.insert(CritMapVal(cr->GetId(),cr));
	if(cr->IsChosen()) chosenId=cr->GetId();
	SetCrit(cr);
}

void HexManager::EraseCrit(DWORD crid)
{
	CritMapIt it=allCritters.find(crid);
	if(it==allCritters.end()) return;
	CritterCl* cr=(*it).second;
	if(cr->IsChosen()) chosenId=0;
	RemoveCrit(cr);
	cr->EraseAllItems();
	cr->IsNotValid=true;
	cr->Release();
	allCritters.erase(it);
}

void HexManager::ClearCritters()
{
	for(CritMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		CritterCl* cr=(*it).second;
		RemoveCrit(cr);
		cr->EraseAllItems();
		cr->IsNotValid=true;
		cr->Release();
	}
	allCritters.clear();
	chosenId=0;
}

void HexManager::GetCritters(WORD hx, WORD hy, CritVec& crits, int find_type)
{
	Field* f=&GetField(hx,hy);
	if(f->Crit && f->Crit->CheckFind(find_type)) crits.push_back(f->Crit);
	for(CritVecIt it=f->DeadCrits.begin(),end=f->DeadCrits.end();it!=end;++it) if((*it)->CheckFind(find_type)) crits.push_back(*it);
}

void HexManager::SetCritterContour(DWORD crid, Sprite::ContourType contour)
{
	if(critterContourCrId)
	{
		CritterCl* cr=GetCritter(critterContourCrId);
		if(cr && cr->SprDrawValid)
		{
			if(!cr->IsDead() && !cr->IsChosen()) cr->SprDraw->Contour=crittersContour;
			else cr->SprDraw->Contour=Sprite::ContourNone;
		}
	}
	critterContourCrId=crid;
	critterContour=contour;
	if(crid)
	{
		CritterCl* cr=GetCritter(crid);
		if(cr && cr->SprDrawValid) cr->SprDraw->Contour=contour;
	}
}

void HexManager::SetCrittersContour(Sprite::ContourType contour)
{
	if(crittersContour==contour) return;
	crittersContour=contour;
	for(CritMapIt it=allCritters.begin(),end=allCritters.end();it!=end;it++)
	{
		CritterCl* cr=(*it).second;
		if(!cr->IsChosen() && cr->SprDrawValid && !cr->IsDead() && cr->GetId()!=critterContourCrId) cr->SprDraw->Contour=contour;
	}
}

bool HexManager::TransitCritter(CritterCl* cr, int hx, int hy, bool animate, bool force)
{
	if(!IsMapLoaded() || hx<0 || hx>=maxHexX || hy<0 || hy>=maxHexY) return false;
	if(cr->GetHexX()==hx && cr->GetHexY()==hy) return true;

	if(cr->IsDead())
	{
		RemoveCrit(cr);
		cr->HexX=hx;
		cr->HexY=hy;
		SetCrit(cr);

		if(cr->IsChosen() && cr->Visible) RebuildLight();
		return true;
	}

	Field& f=GetField(hx,hy);

	if(f.Crit!=NULL)
	{
		if(force && f.Crit->IsLastHexes()) TransitCritter(f.Crit,f.Crit->PopLastHexX(),f.Crit->PopLastHexY(),false,true);
		if(f.Crit!=NULL)
		{
			//cr->ZeroSteps(); Try move every frame
			return false;
		}
	}

	int old_hx=cr->GetHexX();
	int old_hy=cr->GetHexY();
	cr->HexX=hx;
	cr->HexY=hy;
	int dir=GetFarDir(old_hx,old_hy,hx,hy);

	if(animate)
	{
		cr->Move(dir);
		if(DistGame(old_hx,old_hy,hx,hy)>1)
		{
			WORD hx_=hx;
			WORD hy_=hy;
			MoveHexByDir(hx_,hy_,ReverseDir(dir),maxHexX,maxHexY);
			int ox,oy;
			GetHexOffset(hx_,hy_,old_hx,old_hy,ox,oy);
			cr->AddOffsExt(ox,oy);
		}
	}
	else
	{
		int ox,oy;
		GetHexOffset(hx,hy,old_hx,old_hy,ox,oy);
		cr->AddOffsExt(ox,oy);
	}

	GetField(old_hx,old_hy).Crit=NULL;
	f.Crit=cr;

	if(cr->IsChosen() && cr->Visible) RebuildLight();
	if(cr->SprDrawValid)  cr->SprDraw->Unvalidate();

	if(GetHexToDraw(hx,hy) && cr->Visible)
	{
		Sprite& spr=mainTree.InsertSprite(DRAW_ORDER_CRIT(f.Pos),
			f.ScrX+HEX_OX,f.ScrY+HEX_OY,0,&cr->SprId,&cr->SprOx,&cr->SprOy,
			&cr->Alpha,GetLightHex(hx,hy),&cr->SprDrawValid);
		cr->SprDraw=&spr;

		/*if(CheckDist(old_hx,old_hy,hx,hy,1))
		{
			if(old_hx%2 && dir==0) pos=DRAW_ORDER_CRIT(f.Pos+11);
			else if(!(old_hx%2) && dir==3) pos=DRAW_ORDER_CRIT(HexField[old_hy][old_hx].Pos+12);
			else if(old_hx%2 && dir==2) pos=DRAW_ORDER_CRIT(HexField[old_hy][old_hx].Pos+1);
			else if(!(old_hx%2) && dir==5) pos=DRAW_ORDER_CRIT(HexField[old_hy][old_hx].Pos+2);
		}*/

		cr->SetSprRect();
		cr->FixLastHexes();
		if(cr->GetId()==critterContourCrId) spr.Contour=critterContour;
		else if(!cr->IsDead() && !cr->IsChosen()) spr.Contour=crittersContour;
		spr.ContourColor=cr->ContourColor;
	}

	GetField(old_hx,old_hy).ProcessCache();
	f.ProcessCache();
	return true;
}

bool HexManager::GetHexPixel(int x, int y, WORD& hx, WORD& hy)
{
	if(!IsMapLoaded()) return false;

	int y2=0;
	int vpos=0;

	for(int ty=0;ty<hVisible;ty++)
	{
		for(int tx=0;tx<wVisible;tx++)
		{
			vpos=y2+tx;

			int x_=viewField[vpos].ScrX/ZOOM;
			int y_=viewField[vpos].ScrY/ZOOM;

			if(x<=x_) continue;
			if(x>x_+32/ZOOM) continue;
			if(y<=y_) continue;
			if(y>y_+16/ZOOM) continue;

			hx=viewField[vpos].HexX;
			hy=viewField[vpos].HexY;

			if(hx>=maxHexX || hy>=maxHexY) return false;
			return true;
		}
		y2+=wVisible;
	}

	return false;
}

bool CompItemPos(ItemHex* o1, ItemHex* o2)
{
	DWORD pos1=(o1->IsFlat()?DRAW_ORDER_ITEM_FLAT(o1->IsScenOrGrid()):DRAW_ORDER_ITEM(o1->GetPos()));
	DWORD pos2=(o2->IsFlat()?DRAW_ORDER_ITEM_FLAT(o2->IsScenOrGrid()):DRAW_ORDER_ITEM(o2->GetPos()));
	return pos1>=pos2;
}

bool CompItemTransp(ItemHex* o1, ItemHex* o2)
{
	return !o1->IsTransparent() && o2->IsTransparent();
}

ItemHex* HexManager::GetItemPixel(int x, int y, bool& item_egg)
{
	if(!IsMapLoaded()) return NULL;

	ItemHexVec pix_item;
	ItemHexVec pix_item_egg;
	bool is_egg=sprMngr->IsEggTransp(x,y);
	int ehx,ehy;
	sprMngr->GetEggPos(ehx,ehy);

	for(ItemHexVecIt it=hexItems.begin(),end=hexItems.end();it!=end;++it)
	{
		ItemHex* item=(*it);
		WORD hx=item->GetHexX();
		WORD hy=item->GetHexY();

		if(item->IsFinishing()) continue;

#ifdef FONLINE_CLIENT
		if(item->IsHidden() || item->Proto->TransVal==0xFF) continue;
		if(item->IsScenOrGrid() && !OptShowScen) continue;
		if(item->IsItem() && !OptShowItem) continue;
		if(item->IsWall() && !OptShowWall) continue;
#else // FONLINE_MAPPER
		bool is_fast=fastPids.count(item->GetProtoId())!=0;
		if(item->IsScenOrGrid() && !OptShowScen && !is_fast) continue;
		if(item->IsItem() && !OptShowItem && !is_fast) continue;
		if(item->IsWall() && !OptShowWall && !is_fast) continue;
		if(!OptShowFast && is_fast) continue;
		if(ignorePids.count(item->GetProtoId())) continue;
#endif

		SpriteInfo* sprinf=sprMngr->GetSpriteInfo(item->SprId);
		if(!sprinf) continue;

		int l=(*item->HexScrX+item->ScrX+sprinf->OffsX+16+CmnScrOx-sprinf->Width/2)/ZOOM;
		int r=(*item->HexScrX+item->ScrX+sprinf->OffsX+16+CmnScrOx+sprinf->Width/2)/ZOOM;
		int t=(*item->HexScrY+item->ScrY+sprinf->OffsY+6+CmnScrOy-sprinf->Height)/ZOOM;
		int b=(*item->HexScrY+item->ScrY+sprinf->OffsY+6+CmnScrOy)/ZOOM;

		if(x>=l && x<=r && y>=t && y<=b && sprMngr->IsPixNoTransp(item->SprId,x-l,y-t))
		{
			if(is_egg && sprMngr->CompareHexEgg(hx,hy,item->GetEggType())) pix_item_egg.push_back(item);
			else pix_item.push_back(item);
		}
	}

	// Egg items
	if(pix_item.empty())
	{
		if(pix_item_egg.empty()) return NULL;
		if(pix_item_egg.size()>1)
		{
			std::sort(pix_item_egg.begin(),pix_item_egg.end(),CompItemPos);
			std::sort(pix_item_egg.begin(),pix_item_egg.end(),CompItemTransp);
		}
		item_egg=true;
		return pix_item_egg[0];
	}

	// Visible items
	if(pix_item.size()>1)
	{
		std::sort(pix_item.begin(),pix_item.end(),CompItemPos);
		std::sort(pix_item.begin(),pix_item.end(),CompItemTransp);
	}
	item_egg=false;
	return pix_item[0];
}

bool CompCritsPos(CritterCl* cr1, CritterCl* cr2)
{
	DWORD pos1=(cr1->IsDead()?DRAW_ORDER_CRIT_DEAD(cr1->GetPos()):DRAW_ORDER_CRIT(cr1->GetPos()));
	DWORD pos2=(cr2->IsDead()?DRAW_ORDER_CRIT_DEAD(cr2->GetPos()):DRAW_ORDER_CRIT(cr2->GetPos()));
	return pos1>=pos2;
}

CritterCl* HexManager::GetCritterPixel(int x, int y, bool ignor_mode)
{
	if(!IsMapLoaded() || !OptShowCrit) return NULL;

	CritVec crits;
	for(CritMapIt it=allCritters.begin();it!=allCritters.end();it++)
	{
		CritterCl* cr=(*it).second;
		if(!cr->Visible || cr->IsFinishing()) continue;
		if(ignor_mode && (cr->IsDead() || cr->IsChosen())) continue;

		if(cr->Anim3d)
		{
			if(cr->Anim3d->IsIntersect(x,y)) crits.push_back(cr);
			continue;
		}

		if(x>=(cr->DRect.L+CmnScrOx)/ZOOM && x<=(cr->DRect.R+CmnScrOx)/ZOOM &&
		   y>=(cr->DRect.T+CmnScrOy)/ZOOM && y<=(cr->DRect.B+CmnScrOy)/ZOOM &&
			sprMngr->IsPixNoTransp(cr->SprId,x-(cr->DRect.L+CmnScrOx)/ZOOM,y-(cr->DRect.T+CmnScrOy)/ZOOM))
		{
			crits.push_back(cr);
		}
	}

	if(crits.empty()) return NULL;
	if(crits.size()>1) std::sort(crits.begin(),crits.end(),CompCritsPos);
	return crits[0];
}

void HexManager::GetSmthPixel(int pix_x, int pix_y, ItemHex*& item, CritterCl*& cr)
{
	bool item_egg;
	item=GetItemPixel(pix_x,pix_y,item_egg);
	cr=GetCritterPixel(pix_x,pix_y,false);

	if(cr && item)
	{
		if(item->IsTransparent() || item_egg) item=NULL;
		else
		{
			DWORD pos_cr=(cr->IsDead()?DRAW_ORDER_CRIT_DEAD(cr->GetPos()):DRAW_ORDER_CRIT(cr->GetPos()));
			DWORD pos_item=(item->IsFlat()?DRAW_ORDER_ITEM_FLAT(item->IsScenOrGrid()):DRAW_ORDER_ITEM(item->GetPos()));
			if(pos_cr>=pos_item) item=NULL;
			else cr=NULL;
		}
	}
}

int GridOffsX=0,GridOffsY=0;
short Grid[FINDPATH_MAX_PATH*2+2][FINDPATH_MAX_PATH*2+2];
#define GRID(x,y) Grid[(FINDPATH_MAX_PATH+1)+(x)-GridOffsX][(FINDPATH_MAX_PATH+1)+(y)-GridOffsY]
int HexManager::FindStep(WORD start_x, WORD start_y, WORD end_x, WORD end_y, ByteVec& steps)
{
	if(!IsMapLoaded()) return FP_ERROR;
	if(start_x==end_x && start_y==end_y) return FP_ALREADY_HERE;

	short numindex=1;
	ZeroMemory(Grid,sizeof(Grid));
	GridOffsX=start_x;
	GridOffsY=start_y;
	GRID(start_x,start_y)=numindex;

	WordPairVec coords;
	coords.reserve(FINDPATH_MAX_PATH*4);
	coords.push_back(WordPairVecVal(start_x,start_y));

	DWORD p=0,p_togo=0;
	while(true)
	{
		if(++numindex>FINDPATH_MAX_PATH) return FP_TOOFAR;
		if(!(p_togo=coords.size()-p)) return FP_DEADLOCK;

		for(int i=0;i<p_togo;++i,++p)
		{
			WORD cx=coords[p].first;
			WORD cy=coords[p].second;

			for(int j=1;j<=6;++j)
			{
				WORD nx=cx+((cx%2)?SXNChet[j]:SXChet[j]);
				WORD ny=cy+((cx%2)?SYNChet[j]:SYChet[j]);
				if(nx>=maxHexX || ny>=maxHexY) continue;

				if(!GRID(nx,ny) && !GetField(nx,ny).IsNotPassed)
				{
					GRID(nx,ny)=numindex;
					coords.push_back(WordPairVecVal(nx,ny));

					if(nx==end_x && ny==end_y) goto label_FindOk;
				}
			}
		}
	}

label_FindOk:
	int x1=coords[coords.size()-1].first;
	int y1=coords[coords.size()-1].second;
	steps.resize(numindex-1);

	// From end
	static bool switcher=false;
	while(numindex>1)
	{
		if(numindex%2) switcher=!switcher;
		numindex--;

		if(switcher)
		{
			if(x1%2)
			{
				if(GRID(x1-1,y1-1)==numindex) { steps[numindex-1]=GetDir(x1-1,y1-1,x1,y1); x1--; y1--; continue; } // 0
				if(GRID(x1,y1-1)==numindex)   { steps[numindex-1]=GetDir(x1,y1-1,x1,y1); y1--; continue; } // 5
				if(GRID(x1,y1+1)==numindex)   { steps[numindex-1]=GetDir(x1,y1+1,x1,y1); y1++; continue; } // 2
				if(GRID(x1+1,y1)==numindex)   { steps[numindex-1]=GetDir(x1+1,y1,x1,y1); x1++; continue; } // 3
				if(GRID(x1-1,y1)==numindex)   { steps[numindex-1]=GetDir(x1-1,y1,x1,y1); x1--; continue; } // 1
				if(GRID(x1+1,y1-1)==numindex) { steps[numindex-1]=GetDir(x1+1,y1-1,x1,y1); x1++; y1--; continue; } // 4
			}
			else
			{
				if(GRID(x1-1,y1)==numindex)   { steps[numindex-1]=GetDir(x1-1,y1,x1,y1); x1--; continue; } // 0
				if(GRID(x1,y1-1)==numindex)   { steps[numindex-1]=GetDir(x1,y1-1,x1,y1); y1--; continue; } // 5
				if(GRID(x1,y1+1)==numindex)   { steps[numindex-1]=GetDir(x1,y1+1,x1,y1); y1++; continue; } // 2
				if(GRID(x1+1,y1+1)==numindex) { steps[numindex-1]=GetDir(x1+1,y1+1,x1,y1); x1++; y1++; continue; } // 3
				if(GRID(x1-1,y1+1)==numindex) { steps[numindex-1]=GetDir(x1-1,y1+1,x1,y1); x1--; y1++; continue; } // 1
				if(GRID(x1+1,y1)==numindex)   { steps[numindex-1]=GetDir(x1+1,y1,x1,y1); x1++; continue; } // 4
			}
		}
		else
		{
			if(x1%2)
			{
				if(GRID(x1-1,y1)==numindex)   { steps[numindex-1]=GetDir(x1-1,y1,x1,y1); x1--; continue; } // 1
				if(GRID(x1+1,y1-1)==numindex) { steps[numindex-1]=GetDir(x1+1,y1-1,x1,y1); x1++; y1--; continue; } // 4
				if(GRID(x1,y1-1)==numindex)   { steps[numindex-1]=GetDir(x1,y1-1,x1,y1); y1--; continue; } // 5
				if(GRID(x1-1,y1-1)==numindex) { steps[numindex-1]=GetDir(x1-1,y1-1,x1,y1); x1--; y1--; continue; } // 0
				if(GRID(x1+1,y1)==numindex)   { steps[numindex-1]=GetDir(x1+1,y1,x1,y1); x1++; continue; } // 3
				if(GRID(x1,y1+1)==numindex)   { steps[numindex-1]=GetDir(x1,y1+1,x1,y1); y1++; continue; } // 2
			}
			else
			{
				if(GRID(x1-1,y1+1)==numindex) { steps[numindex-1]=GetDir(x1-1,y1+1,x1,y1); x1--; y1++; continue; } // 1
				if(GRID(x1+1,y1)==numindex)   { steps[numindex-1]=GetDir(x1+1,y1,x1,y1); x1++; continue; } // 4
				if(GRID(x1,y1-1)==numindex)   { steps[numindex-1]=GetDir(x1,y1-1,x1,y1); y1--; continue; } // 5
				if(GRID(x1-1,y1)==numindex)   { steps[numindex-1]=GetDir(x1-1,y1,x1,y1); x1--; continue; } // 0
				if(GRID(x1+1,y1+1)==numindex) { steps[numindex-1]=GetDir(x1+1,y1+1,x1,y1); x1++; y1++; continue; } // 3
				if(GRID(x1,y1+1)==numindex)   { steps[numindex-1]=GetDir(x1,y1+1,x1,y1); y1++; continue; } // 2
			}
		}
		return FP_ERROR;
	}
	return FP_OK;
}

int HexManager::CutPath(WORD start_x, WORD start_y, WORD& end_x, WORD& end_y, int cut)
{
	if(!IsMapLoaded()) return FP_ERROR;

	short numindex=1;
	ZeroMemory(Grid,sizeof(Grid));
	GridOffsX=start_x;
	GridOffsY=start_y;
	GRID(start_x,start_y)=numindex;
	WordPairVec path;
	path.reserve(FINDPATH_MAX_PATH*4);
	path.push_back(WordPairVecVal(start_x,start_y));

	size_t p=0;
	size_t p_togo=0;
	while(true)
	{
		if(++numindex>FINDPATH_MAX_PATH) return FP_TOOFAR;
		if(!(p_togo=path.size()-p)) return FP_DEADLOCK;

		for(size_t p_cur=0;p_cur<p_togo;++p_cur,++p)
		{
			WORD cx=path[p].first;
			WORD cy=path[p].second;

			for(int i=1;i<=6;++i)
			{
				WORD nx=cx+((cx%2)?SXNChet[i]:SXChet[i]);
				WORD ny=cy+((cx%2)?SYNChet[i]:SYChet[i]);
				if(nx>=maxHexX || ny>=maxHexY) continue;

				if(!GRID(nx,ny) && !GetField(nx,ny).IsNotPassed)
				{
					GRID(nx,ny)=numindex;
					path.push_back(WordPairVecVal(nx,ny));

					if(CheckDist(nx,ny,end_x,end_y,cut))
					{
						end_x=nx;
						end_y=ny;
						return FP_OK;
					}
				}
			}
		}
	}
	return FP_ERROR;
}

bool HexManager::TraceBullet(WORD hx, WORD hy, WORD tx, WORD ty, int dist, float angle, CritterCl* find_cr, bool find_cr_safe, CritVec* critters, int find_type, WordPair* pre_block, WordPair* block, WordPairVec* steps, bool check_passed)
{
	if(IsShowTrack()) ClearHexTrack();

	if(!dist) dist=DistGame(hx,hy,tx,ty);
	float nx=3.0f*(float(tx)-float(hx));
	float ny=(float(ty)-float(hy))*SQRT3T2_FLOAT-(float(tx%2)-float(hx%2))*SQRT3_FLOAT;
	float dir=180.0f+RAD2DEG*atan2f(ny,nx);

	if(angle!=0.0f)
	{
		dir+=angle;
		if(dir<0.0f) dir+=360.0f;
		else if(dir>360.0f) dir-=360.0f;
	}

	BYTE dir1,dir2;
	if(dir>=30.0f && dir<90.0f) { dir1=5; dir2=0; }
	else if(dir>=90.0f && dir<150.0f) { dir1=4; dir2=5; }
	else if(dir>=150.0f && dir<210.0f) { dir1=3; dir2=4; }
	else if(dir>=210.0f && dir<270.0f) { dir1=2; dir2=3; }
	else if(dir>=270.0f && dir<330.0f) { dir1=1; dir2=2; }
	else { dir1=0; dir2=1; }

	WORD cx=hx;
	WORD cy=hy;
	WORD old_cx=cx;
	WORD old_cy=cy;
	WORD t1x,t1y,t2x,t2y;

	float x1=3.0f*float(hx)+BIAS_FLOAT;
	float y1=SQRT3T2_FLOAT*float(hy)-SQRT3_FLOAT*(float(hx%2))+BIAS_FLOAT;
	float x2=3.0f*float(tx)+BIAS_FLOAT+BIAS_FLOAT;
	float y2=SQRT3T2_FLOAT*float(ty)-SQRT3_FLOAT*(float(tx%2))+BIAS_FLOAT;
	if(angle!=0.0f)
	{
		x2-=x1;
		y2-=y1;
		float xp=cos(angle/RAD2DEG)*x2-sin(angle/RAD2DEG)*y2;
		float yp=sin(angle/RAD2DEG)*x2+cos(angle/RAD2DEG)*y2;
		x2=x1+xp;
		y2=y1+yp;
	}

	float dx=x2-x1;
	float dy=y2-y1;
	float c1x,c1y,c2x,c2y;
	float dist1,dist2;
	for(int i=0;i<dist;i++)
	{
		t1x=cx;
		t2x=cx;
		t1y=cy;
		t2y=cy;
		MoveHexByDir(t1x,t1y,dir1,maxHexX,maxHexY);
		MoveHexByDir(t2x,t2y,dir2,maxHexX,maxHexY);
		c1x=3.0f*float(t1x);
		c1y=SQRT3T2_FLOAT*float(t1y)-(float(t1x%2))*SQRT3_FLOAT;
		c2x=3.0f*float(t2x);
		c2y=SQRT3T2_FLOAT*float(t2y)-(float(t2x%2))*SQRT3_FLOAT;
		dist1=dx*(y1-c1y)-dy*(x1-c1x);
		dist2=dx*(y1-c2y)-dy*(x1-c2x);
		dist1=(dist1>0?dist1:-dist1);
		dist2=(dist2>0?dist2:-dist2);
		if(dist1<=dist2) // Left hand biased
		{
			cx=t1x;
			cy=t1y;
		}
		else
		{
			cx=t2x;
			cy=t2y;
		}

		if(IsShowTrack()) GetHexTrack(cx,cy)=(cx==tx && cy==ty?1:2);

		if(steps)
		{
			steps->push_back(WordPairVecVal(cx,cy));
			continue;
		}

		Field& f=GetField(cx,cy);
		if(check_passed && f.IsNotRaked) break;
		if(critters!=NULL) GetCritters(cx,cy,*critters,find_type);
		if(find_cr!=NULL && f.Crit)
		{
			CritterCl* cr=f.Crit;
			if(cr && cr==find_cr) return true;
			if(find_cr_safe) return false;
		}

		old_cx=cx;
		old_cy=cy;
	}

	if(pre_block)
	{
		(*pre_block).first=old_cx;
		(*pre_block).second=old_cy;
	}
	if(block)
	{
		(*block).first=cx;
		(*block).second=cy;
	}
	return false;
}

void HexManager::FindSetCenter(int x, int y)
{
	if(!viewField) return;
	RebuildMap(x,y);

#ifdef FONLINE_CLIENT
	int iw=VIEW_WIDTH/2+2;
	int ih=VIEW_HEIGHT/2+2;
	WORD hx=x;
	WORD hy=y;
	ByteVec dirs;

	// Up
	dirs.clear();
	dirs.push_back(0);
	dirs.push_back(5);
	FindSetCenterDir(hx,hy,dirs,ih);
	// Down
	dirs.clear();
	dirs.push_back(3);
	dirs.push_back(2);
	FindSetCenterDir(hx,hy,dirs,ih);
	// Left
	dirs.clear();
	dirs.push_back(1);
	FindSetCenterDir(hx,hy,dirs,iw);
	// Right
	dirs.clear();
	dirs.push_back(4);
	FindSetCenterDir(hx,hy,dirs,iw);

	// Up-Right
	dirs.clear();
	dirs.push_back(0);
	FindSetCenterDir(hx,hy,dirs,ih);
	// Down-Left
	dirs.clear();
	dirs.push_back(3);
	FindSetCenterDir(hx,hy,dirs,ih);
	// Up-Left
	dirs.clear();
	dirs.push_back(5);
	FindSetCenterDir(hx,hy,dirs,ih);
	// Down-Right
	dirs.clear();
	dirs.push_back(2);
	FindSetCenterDir(hx,hy,dirs,ih);

	RebuildMap(hx,hy);
#endif //FONLINE_CLIENT
}

void HexManager::FindSetCenterDir(WORD& x, WORD& y, ByteVec& dirs, int steps)
{
	if(dirs.empty()) return;

	WORD sx=x;
	WORD sy=y;
	int cur_dir=0;

	int i;
	for(i=0;i<steps;i++)
	{
		MoveHexByDir(sx,sy,dirs[cur_dir],maxHexX,maxHexY);
		cur_dir++;
		if(cur_dir>=dirs.size()) cur_dir=0;

		GetHexTrack(sx,sy)=1;
		if(GetField(sx,sy).ScrollBlock) break;
		GetHexTrack(sx,sy)=2;
	}

	for(;i<steps;i++)
	{
		MoveHexByDir(x,y,ReverseDir(dirs[cur_dir]),maxHexX,maxHexY);
		cur_dir++;
		if(cur_dir>=dirs.size()) cur_dir=0;
	}
}

bool HexManager::LoadMap(WORD pid_map)
{
	WriteLog("Load FO map...");
	UnLoadMap();

	char map_name[256];
	sprintf(map_name,"map%u",pid_map);

	DWORD cache_len;
	BYTE* cache=Crypt.GetCache(map_name,cache_len);

	if(!cache)
	{
		WriteLog("Load map<%s> from cache fail.\n",map_name);
		return false;
	}

	FileManager fm;
	if(!fm.LoadStream(cache,cache_len))
	{
		WriteLog("Load map<%s> from stream fail.\n",map_name);
		delete[] cache;
		return false;
	}
	delete[] cache;

	// Header
	DWORD buf_len=fm.GetFsize();
	BYTE* buf=Crypt.Uncompress(fm.GetBuf(),buf_len,50);
	if(!buf)
	{
		WriteLog("Uncompress map fail.\n");
		return false;
	}

	fm.UnloadFile();
	fm.LoadStream(buf,buf_len);
	delete[] buf;

	if(fm.GetBEDWord()!=CLIENT_MAP_FORMAT_VER)
	{
		WriteLog("Map Format is not supported.\n");
		return false;
	}

	if(fm.GetBEDWord()!=pid_map)
	{
		WriteLog("Pid Map != Name Map.\n");
		return false;
	}

	// Reserved
	WORD maxhx=fm.GetBEWord();
	if(maxhx==0xAABB) maxhx=400;
	WORD maxhy=fm.GetBEWord();
	if(maxhy==0xCCDD) maxhy=400;
	fm.GetBEDWord();
	fm.GetBEDWord();

	DWORD tiles_count=fm.GetBEDWord();
	DWORD walls_count=fm.GetBEDWord();
	DWORD scen_count=fm.GetBEDWord();
	DWORD tiles_len=fm.GetBEDWord();
	DWORD walls_len=fm.GetBEDWord();
	DWORD scen_len=fm.GetBEDWord();

	if(tiles_count*sizeof(DWORD)*2!=tiles_len)
	{
		WriteLog("Tiles data truncated.\n");
		return false;
	}

	if(walls_count*sizeof(ScenToSend)!=walls_len)
	{
		WriteLog("Walls data truncated.\n");
		return false;
	}

	if(scen_count*sizeof(ScenToSend)!=scen_len)
	{
		WriteLog("Scenery data truncated.\n");
		return false;
	}

	// Create field
	if(!ResizeField(maxhx,maxhy))
	{
		WriteLog("Buffer allocation fail.\n");
		return false;
	}

	// Tiles
	fm.SetCurPos(0x2C);
	curHashTiles=maxhx*maxhy;
	Crypt.Crc32(fm.GetCurBuf(),tiles_len,curHashTiles);

	for(int ty=0,ey=maxHexY/2;ty<ey;ty++)
	{
		for(int tx=0,ex=maxHexX/2;tx<ex;tx++)
		{
			DWORD tile=fm.GetLEDWord();
			DWORD roof=fm.GetLEDWord();
			Field& f=GetField(tx*2,ty*2);
			f.TileId=0;
			f.RoofId=0;

			if(tile)
			{
				DWORD id=ResMngr.GetSprId(tile);
				if(id)
				{
					f.TileId=id;
					CheckTilesBorder(id,false);
				}
			}

			if(roof)
			{
				DWORD id=ResMngr.GetSprId(roof);
				if(id)
				{
					f.RoofId=id;
					CheckTilesBorder(id,true);
				}
			}
		}
	}

	// Roof indexes
	int roof_num=1;
	for(int ty=0,ey=maxHexY/2;ty<ey;ty++)
	{
		for(int tx=0,ex=maxHexX/2;tx<ex;tx++)
		{
			WORD rtile=GetField(tx*2,ty*2).RoofId;
			if(rtile)
			{
				MarkRoofNum(tx*2,ty*2,roof_num);
				roof_num++;
			}
		}
	}

	// Walls
	fm.SetCurPos(0x2C+tiles_len);
	curHashWalls=maxhx*maxhy;
	Crypt.Crc32(fm.GetCurBuf(),walls_len,curHashWalls);

	while(walls_count)
	{
		walls_count--;

		ScenToSend cur_wall;
		ZeroMemory(&cur_wall,sizeof(cur_wall));

		if(!fm.CopyMem(&cur_wall,sizeof(cur_wall)))
		{
			WriteLog("Unable to read wall item.\n");
			continue;
		}

		Crypt.Crc32((BYTE*)&cur_wall,sizeof(cur_wall),curHashWalls);

		if(!ParseScenery(cur_wall))
		{
			WriteLog("Unable to parse wall item.\n");
			continue;
		}
	}

	// Scenery
	fm.SetCurPos(0x2C+tiles_len+walls_len);
	curHashScen=maxhx*maxhy;
	Crypt.Crc32(fm.GetCurBuf(),scen_len,curHashScen);

	while(scen_count)
	{
		scen_count--;

		ScenToSend cur_scen;
		ZeroMemory(&cur_scen,sizeof(cur_scen));

		if(!fm.CopyMem(&cur_scen,sizeof(cur_scen)))
		{
			WriteLog("Unable to read scenery item.\n");
			continue;
		}

		if(!ParseScenery(cur_scen))
		{
			WriteLog("Unable to parse scenery item<%d>.\n",cur_scen.ProtoId);
			continue;
		}
	}

	// Scroll blocks borders
	for(int hx=0;hx<maxHexX;hx++)
	{
		for(int hy=0;hy<maxHexY;hy++)
		{
			Field& f=GetField(hx,hy);
			if(f.ScrollBlock)
			{
				for(int i=0;i<6;i++)
				{
					WORD hx_=hx,hy_=hy;
					MoveHexByDir(hx_,hy_,i,maxHexX,maxHexY);
					GetField(hx_,hy_).IsNotPassed=true;
				}
			}
		}
	}

	// Light
	CollectLightSources();
	lightPoints.clear();
	lightPointsCount=0;

	// Visible
	hVisible=VIEW_HEIGHT+hTop+hBottom;
	wVisible=VIEW_WIDTH+wLeft+wRight;
	SAFEDELA(viewField);
	viewField=new ViewField[hVisible*wVisible];

	// Finish
	curPidMap=pid_map;
	curMapTime=-1;
	AutoScroll.Active=false;
	WriteLog("Load FO map success.\n");
	return true;
}

void HexManager::UnLoadMap()
{
	if(!IsMapLoaded()) return;

	SAFEREL(tileVB);
	SAFEDELA(viewField);

	hTop=0;
	hBottom=0;
	wLeft=0;
	wRight=0;
	hVisible=0;
	wVisible=0;
	screenHexX=0;
	screenHexY=0;

	lightSources.clear();
	lightSourcesScen.clear();

	mainTree.Unvalidate();
	roofTree.Unvalidate();
	roofRainTree.Unvalidate();

	for(DropVecIt it=rainData.begin(),end=rainData.end();it!=end;++it)
		delete *it;
	rainData.clear();

	for(int i=0,j=hexItems.size();i<j;i++)
		hexItems[i]->Release();
	hexItems.clear();

	ResizeField(0,0);
	ClearCritters();

	curPidMap=0;
	curMapTime=-1;
	curHashTiles=0;
	curHashWalls=0;
	curHashScen=0;

	crittersContour=Sprite::ContourNone;
	critterContour=Sprite::ContourNone;
}

void HexManager::GetMapHash(WORD pid_map, DWORD& hash_tiles, DWORD& hash_walls, DWORD& hash_scen)
{
	WriteLog("Get hash of map, pid<%u>...",pid_map);

	hash_tiles=0;
	hash_walls=0;
	hash_scen=0;

	if(pid_map==curPidMap)
	{
		hash_tiles=curHashTiles;
		hash_walls=curHashWalls;
		hash_scen=curHashScen;

		WriteLog("Hashes of loaded map: tiles<%u>, walls<%u>, scenery<%u>.\n",hash_tiles,hash_walls,hash_scen);
		return;
	}

	char map_name[256];
	sprintf(map_name,"map%u",pid_map);

	DWORD cache_len;
	BYTE* cache=Crypt.GetCache(map_name,cache_len);
	if(!cache)
	{
		WriteLog("Load map<%s> from cache fail.\n",map_name);
		return;
	}

	if(!fmMap.LoadStream(cache,cache_len))
	{
		WriteLog("Load map<%s> from stream fail.\n",map_name);
		delete[] cache;
		return;
	}
	delete[] cache;

	DWORD buf_len=fmMap.GetFsize();
	BYTE* buf=Crypt.Uncompress(fmMap.GetBuf(),buf_len,50);
	if(!buf)
	{
		WriteLog("Uncompress map fail.\n");
		return;
	}

	fmMap.UnloadFile();
	fmMap.LoadStream(buf,buf_len);
	delete[] buf;

	if(fmMap.GetBEDWord()!=CLIENT_MAP_FORMAT_VER)
	{
		WriteLog("Map format is not supported.\n");
		fmMap.UnloadFile();
		return;
	}

	if(fmMap.GetBEDWord()!=pid_map)
	{
		WriteLog("Invalid proto number.\n");
		fmMap.UnloadFile();
		return;
	}

	// Reserved
	WORD maxhx=fmMap.GetBEWord();
	if(maxhx==0xAABB) maxhx=400;
	WORD maxhy=fmMap.GetBEWord();
	if(maxhy==0xCCDD) maxhy=400;
	fmMap.GetBEDWord();
	fmMap.GetBEDWord();
	DWORD tiles_count=fmMap.GetBEDWord();
	DWORD walls_count=fmMap.GetBEDWord();
	DWORD scen_count=fmMap.GetBEDWord();
	DWORD tiles_len=fmMap.GetBEDWord();
	DWORD walls_len=fmMap.GetBEDWord();
	DWORD scen_len=fmMap.GetBEDWord();

	// Data Check Sum
	if(tiles_count*sizeof(DWORD)*2!=tiles_len)
	{
		WriteLog("Invalid check sum tiles.\n");
		fmMap.UnloadFile();
		return;
	}
	
	if(walls_count*sizeof(ScenToSend)!=walls_len)
	{
		WriteLog("Invalid check sum walls.\n");
		fmMap.UnloadFile();
		return;
	}
	
	if(scen_count*sizeof(ScenToSend)!=scen_len)
	{
		WriteLog("Invalid check sum scenery.\n");
		fmMap.UnloadFile();
		return;
	}

	fmMap.SetCurPos(0x2C);
	hash_tiles=maxhx*maxhy;
	Crypt.Crc32(fmMap.GetCurBuf(),tiles_len,hash_tiles);
	fmMap.SetCurPos(0x2C+tiles_len);
	hash_walls=maxhx*maxhy;
	Crypt.Crc32(fmMap.GetCurBuf(),walls_len,hash_walls);
	fmMap.SetCurPos(0x2C+tiles_len+walls_len);
	hash_scen=maxhx*maxhy;
	Crypt.Crc32(fmMap.GetCurBuf(),scen_len,hash_scen);

	fmMap.UnloadFile();
	WriteLog("Hashes: tiles<%u>, walls<%u>, scenery<%u>.\n",hash_tiles,hash_walls,hash_scen);

}

bool HexManager::ParseScenery(ScenToSend& scen)
{
	WORD pid=scen.ProtoId;
	WORD hx=scen.MapX;
	WORD hy=scen.MapY;

	if(hx>=maxHexX || hy>=maxHexY)
	{
		WriteLog(__FUNCTION__" - Invalid coord<%d,%d>.\n",hx,hy);
		return false;
	}

	ProtoItem* proto_item=ItemMngr.GetProtoItem(pid);
	if(!proto_item)
	{
		WriteLog(__FUNCTION__" - Proto item not found<%d>.\n",pid);
		return false;
	}

	if(scen.LightIntensity) lightSourcesScen.push_back(LightSource(hx,hy,((DWORD)scen.LightR<<16)|scen.LightGB,scen.LightRadius,scen.LightIntensity,scen.LightFlags));

	static DWORD scen_id=0;
	scen_id--;

	ItemHex* scenery=new ItemHex(scen_id,proto_item,NULL,hx,hy,0,scen.OffsetX,scen.OffsetY,&GetField(hx,hy).ScrX,&GetField(hx,hy).ScrY);
	scenery->ScenFlags=scen.Flags;

	// Mapper additional parameters
	if(scen.InfoOffset) scenery->Data.Info=scen.InfoOffset;
	if(scen.AnimStayBegin || scen.AnimStayEnd)
	{
		SETFLAG(scenery->Data.Flags,ITEM_SHOW_ANIM);
		SETFLAG(scenery->Data.Flags,ITEM_SHOW_ANIM_EXT);
		scenery->Data.AnimShow[0]=scen.AnimStayBegin;
		scenery->Data.AnimShow[1]=scen.AnimStayEnd;
		scenery->Data.AnimStay[0]=scen.AnimStayBegin;
		scenery->Data.AnimStay[1]=scen.AnimStayEnd;
		scenery->Data.AnimHide[0]=scen.AnimStayBegin;
		scenery->Data.AnimHide[1]=scen.AnimStayEnd;
	}
	if(scen.AnimWait) scenery->Data.AnimWaitBase=scen.AnimWait;
	if(scen.PicMapHash)
	{
		scenery->Data.PicMapHash=scen.PicMapHash;
		scenery->RefreshAnim();
	}

	PushItem(scenery);
	if(!scenery->IsHidden() && scenery->Proto->TransVal<0xFF) ProcessHexBorders(scenery->Anim->GetSprId(0),scen.OffsetX,scen.OffsetY);
	return true;
}

int HexManager::GetDayTime()
{
	return (GameOpt.Hour*60+GameOpt.Minute);
}

int HexManager::GetMapTime()
{
	if(curMapTime<0) return GetDayTime();
	return curMapTime;
}

int* HexManager::GetMapDayTime()
{
	return dayTime;
}

BYTE* HexManager::GetMapDayColor()
{
	return dayColor;
}

#ifdef FONLINE_MAPPER
bool HexManager::SetProtoMap(ProtoMap& pmap)
{
	WriteLog("Create map from prototype.\n");

	UnLoadMap();
	CurProtoMap=NULL;

	if(!ResizeField(pmap.Header.MaxHexX,pmap.Header.MaxHexY))
	{
		WriteLog("Buffer allocation fail.\n");
		return false;
	}

	for(int i=0;i<4;i++) dayTime[i]=pmap.Header.DayTime[i];
	for(int i=0;i<12;i++) dayColor[i]=pmap.Header.DayColor[i];

	// Tiles
	for(int tx=0;tx<maxHexX/2;tx++)
	{
		for(int ty=0;ty<maxHexY/2;ty++)
		{
			DWORD tile=pmap.GetTile(tx,ty);
			DWORD roof=pmap.GetRoof(tx,ty);

			if(tile)
			{
				DWORD id=ResMngr.GetSprId(tile);
				if(id)
				{
					GetField(tx*2,ty*2).TileId=id;
					CheckTilesBorder(id,false);
				}
			}

			if(roof)
			{
				DWORD id=ResMngr.GetSprId(roof);
				if(id)
				{
					GetField(tx*2,ty*2).RoofId=id;
					CheckTilesBorder(id,true);
				}
			}
		}
	}

	// Objects
	DWORD cur_id=0;
	for(int i=0,j=pmap.MObjects.size();i<j;i++)
	{
		MapObject* o=pmap.MObjects[i];
		if(o->MapX>=maxHexX || o->MapY>=maxHexY)
		{
			WriteLog("Invalid position of map object. Skip.\n");
			continue;
		}

		if(o->MapObjType==MAP_OBJECT_SCENERY)
		{
			ScenToSend s;
			s.ProtoId=o->ProtoId;
			s.MapX=o->MapX;
			s.MapY=o->MapY;
			s.OffsetX=o->MScenery.OffsetX;
			s.OffsetY=o->MScenery.OffsetY;
			s.LightR=(o->LightRGB>>16)&0xFF;
			s.LightGB=o->LightRGB&0xFFFF;
			s.LightRadius=o->LightRadius;
			s.LightFlags=o->LightDirOff|((o->LightDay&3)<<6);
			s.LightIntensity=o->LightIntensity;

			if(!ParseScenery(s))
			{
				WriteLog("Unable to parse scen or wall object.\n");
				continue;
			}
			ItemHex* item=hexItems.back();
			AffectItem(o,item);
			o->RunTime.MapObjId=item->GetId();
		}
		else if(o->MapObjType==MAP_OBJECT_ITEM)
		{
			if(o->MItem.InContainer) continue;

			if(!AddItem(++cur_id,o->ProtoId,o->MapX,o->MapY,false,NULL))
			{
				WriteLog("Unable to parse item object.\n");
				continue;
			}

			ItemHex* item=GetItemById(cur_id);
			if(item) AffectItem(o,item);
			o->RunTime.MapObjId=cur_id;
		}
		else if(o->MapObjType==MAP_OBJECT_CRITTER)
		{
			CritData* pnpc=CrMngr.GetProto(o->ProtoId);
			if(!pnpc)
			{
				WriteLog("Proto<%u> npc not found.\n",o->ProtoId);
				continue;
			}

			ProtoItem* pitem_main=NULL;
			ProtoItem* pitem_ext=NULL;
			ProtoItem* pitem_armor=NULL;
			for(int k=0,l=pmap.MObjects.size();k<l;k++)
			{
				MapObject* child=pmap.MObjects[k];
				if(child->MapObjType==MAP_OBJECT_ITEM && child->MItem.InContainer && child->MapX==o->MapX && child->MapY==o->MapY)
				{
					if(child->MItem.ItemSlot==SLOT_HAND1) pitem_main=ItemMngr.GetProtoItem(child->ProtoId);
					else if(child->MItem.ItemSlot==SLOT_HAND2) pitem_ext=ItemMngr.GetProtoItem(child->ProtoId);
					else if(child->MItem.ItemSlot==SLOT_ARMOR) pitem_armor=ItemMngr.GetProtoItem(child->ProtoId);
				}
			}

			CritterCl* cr=new CritterCl();
			cr->SetBaseType(pnpc->BaseType);
			cr->DefItemSlotMain.Init(pitem_main?pitem_main:ItemMngr.GetProtoItem(ITEM_DEF_SLOT));
			cr->DefItemSlotExt.Init(pitem_ext?pitem_ext:ItemMngr.GetProtoItem(ITEM_DEF_SLOT));
			cr->DefItemSlotArmor.Init(pitem_armor?pitem_armor:ItemMngr.GetProtoItem(ITEM_DEF_ARMOR));
			memcpy(cr->Params,pnpc->Params,sizeof(pnpc->Params));
			cr->HexX=o->MapX;
			cr->HexY=o->MapY;
			cr->Flags=o->ProtoId;
			cr->SetDir(o->Dir);
			cr->Id=++cur_id;
			cr->Init();
			AffectCritter(o,cr);
			AddCrit(cr);
			o->RunTime.MapObjId=cur_id;
		}
	}

	hVisible=VIEW_HEIGHT+hTop+hBottom;
	wVisible=VIEW_WIDTH+wLeft+wRight;
	SAFEDELA(viewField);
	viewField=new ViewField[hVisible*wVisible];

	curPidMap=0xFFFF;
//	curMapTime=pmap.Time;
	curHashTiles=0xFFFF;
	curHashWalls=0xFFFF;
	curHashScen=0xFFFF;
	CurProtoMap=&pmap;
	WriteLog("Create map from prototype complete.\n");
	return true;
}

bool HexManager::GetProtoMap(ProtoMap& pmap)
{
	return false;
}

void HexManager::ParseSelTiles()
{
	bool borders_changed=false;
	for(int tx=0,ex=maxHexX/2;tx<ex;tx++)
	{
		for(int ty=0,ey=maxHexY/2;ty<ey;ty++)
		{
			Field& f=GetField(tx*2,ty*2);
			if(f.SelTile)
			{
				f.TileId=f.SelTile;
				borders_changed=CheckTilesBorder(f.SelTile,false);
			}
			if(f.SelRoof)
			{
				f.RoofId=f.SelRoof;
				borders_changed=CheckTilesBorder(f.SelRoof,true);
			}
		}
	}

	ClearSelTiles();
	if(borders_changed)
	{
		hVisible=VIEW_HEIGHT+hTop+hBottom;
		wVisible=VIEW_WIDTH+wLeft+wRight;
		delete[] viewField;
		viewField=new ViewField[hVisible*wVisible];
		RefreshMap();
	}
}

void HexManager::SetTile(WORD hx, WORD hy, DWORD name_hash, bool is_roof)
{
	DWORD spr_id=ResMngr.GetSprId(name_hash);
	if(is_roof)
	{
		GetField(hx,hy).RoofId=spr_id;
		CurProtoMap->SetRoof(hx/2,hy/2,name_hash);
	}
	else
	{
		GetField(hx,hy).TileId=spr_id;
		CurProtoMap->SetTile(hx/2,hy/2,name_hash);
	}

	if(spr_id && CheckTilesBorder(spr_id,is_roof))
	{
		hVisible=VIEW_HEIGHT+hTop+hBottom;
		wVisible=VIEW_WIDTH+wLeft+wRight;
		delete[] viewField;
		viewField=new ViewField[hVisible*wVisible];
	}
	RefreshMap();
}

void HexManager::AddFastPid(WORD pid)
{
	fastPids.insert(pid);
}

bool HexManager::IsFastPid(WORD pid)
{
	return fastPids.count(pid)!=0;
}

void HexManager::ClearFastPids()
{
	fastPids.clear();
}

void HexManager::SwitchIgnorePid(WORD pid)
{
	if(ignorePids.count(pid)) ignorePids.erase(pid);
	else ignorePids.insert(pid);
}

bool HexManager::IsIgnorePid(WORD pid)
{
	return ignorePids.count(pid)!=0;
}

void HexManager::GetHexesRect(INTRECT& r, WordPairVec& h)
{
	h.clear();

	int x,y;
	GetHexOffset(r[0],r[1],r[2],r[3],x,y);
	x=-x;

	int dx=x/32;
	int dy=y/12;

	int adx=ABS(dx);
	int ady=ABS(dy);

	int hx,hy;
	for(int j=0;j<=ady;j++)
	{
		if(dy>=0)
		{
			hx=r[0]+j/2+j%2;
			hy=r[1]+(j-(hx-r[0]-(r[0]%2?1:0))/2);
		}
		else
		{
			hx=r[0]-j/2-j%2;
			hy=r[1]-(j-(r[0]-hx-(r[0]%2?0:1))/2);
		}

		for(int i=0;i<=adx;i++)
		{
			if(hx<0 || hx>=maxHexX || hy<0 || hy>=maxHexY) continue;

			h.push_back(WordPairVecVal(hx,hy));

			if(dx>=0)
			{
				if(hx%2) hy--;
				hx++;
			}
			else
			{
				hx--;
				if(hx%2) hy++;
			}
		}
	}
}

void HexManager::MarkPassedHexes()
{
	for(int hx=0;hx<maxHexX;hx++)
	{
		for(int hy=0;hy<maxHexY;hy++)
		{
			Field& f=GetField(hx,hy);
			char& track=GetHexTrack(hx,hy);
			track=0;
			if(f.IsNotPassed) track=2;
			if(f.IsNotRaked) track=1;
		}
	}
	RefreshMap();
}

void HexManager::AffectItem(MapObject* mobj, ItemHex* item)
{
	mobj->LightIntensity=CLAMP(mobj->LightIntensity,-100,100);

	if(mobj->MItem.AnimStayBegin || mobj->MItem.AnimStayEnd)
	{
		SETFLAG(item->Data.Flags,ITEM_SHOW_ANIM);
		SETFLAG(item->Data.Flags,ITEM_SHOW_ANIM_EXT);
		item->Data.AnimShow[0]=mobj->MItem.AnimStayBegin;
		item->Data.AnimShow[1]=mobj->MItem.AnimStayEnd;
		item->Data.AnimStay[0]=mobj->MItem.AnimStayBegin;
		item->Data.AnimStay[1]=mobj->MItem.AnimStayEnd;
		item->Data.AnimHide[0]=mobj->MItem.AnimStayBegin;
		item->Data.AnimHide[1]=mobj->MItem.AnimStayEnd;
		item->Data.AnimWaitBase=mobj->MItem.AnimWait;
	}
	else
	{
		item->Data.Flags=item->Proto->Flags;
		item->Data.AnimShow[0]=item->Proto->AnimShow[0];
		item->Data.AnimShow[1]=item->Proto->AnimShow[1];
		item->Data.AnimStay[0]=item->Proto->AnimStay[0];
		item->Data.AnimStay[1]=item->Proto->AnimStay[1];
		item->Data.AnimHide[0]=item->Proto->AnimHide[0];
		item->Data.AnimHide[1]=item->Proto->AnimHide[1];
		item->Data.AnimWaitBase=item->Proto->AnimWaitBase;
	}
	item->Data.LightIntensity=mobj->LightIntensity;
	item->Data.LightColor=mobj->LightRGB;
	item->Data.LightFlags=(mobj->LightDirOff|((mobj->LightDay&3)<<6));
	item->Data.LightRadius=mobj->LightRadius;
	item->Data.PicMapHash=mobj->MItem.PicMapHash;
	item->Data.PicInvHash=mobj->MItem.PicInvHash;
	item->Data.Info=mobj->MItem.InfoOffset;
	item->StartScrX=mobj->MItem.OffsetX;
	item->StartScrY=mobj->MItem.OffsetY;
	if(item->IsHasLocker()) item->Data.Locker.Condition=mobj->MItem.LockerCondition;
	item->RefreshAnim();
}

void HexManager::AffectCritter(MapObject* mobj, CritterCl* cr)
{
	if(mobj->MCritter.Cond<COND_LIFE || mobj->MCritter.Cond>COND_DEAD)
	{
		mobj->MCritter.Cond=COND_LIFE;
		mobj->MCritter.CondExt=COND_LIFE_NONE;
	}

	bool refresh=false;
	if(cr->Cond!=mobj->MCritter.Cond || cr->CondExt!=mobj->MCritter.CondExt) refresh=true;

	cr->Cond=mobj->MCritter.Cond;
	cr->CondExt=mobj->MCritter.CondExt;

	for(int i=0;i<MAPOBJ_CRITTER_PARAMS;i++)
	{
		if(mobj->MCritter.ParamIndex[i]>=0 && mobj->MCritter.ParamIndex[i]<MAX_PARAMS)
			cr->Params[mobj->MCritter.ParamIndex[i]]=mobj->MCritter.ParamValue[i];
	}

	if(refresh) cr->AnimateStay();
}

#endif // FONLINE_MAPPER