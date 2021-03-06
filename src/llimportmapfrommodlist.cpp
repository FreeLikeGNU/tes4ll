#include "../include/llimportmapfrommodlist.h"
#include "../../lltool/include/llmaplist.h"

llImportMapFromModlist::llImportMapFromModlist() : llWorker() {
	SetCommandName("ImportMapFromModlist");
	tes4qlod = NULL;
}

int llImportMapFromModlist::Prepare(void) {
	if (!llWorker::Prepare()) return 0;

	mapname    = NULL;
	opt_size_x = 0;
	opt_size_y = 0;
	opt_center = 0;

	quadoffsetx = quadoffsety = 0;
	autooffset = 0;
	readwaterheight = 0;

	return 1;
}

int llImportMapFromModlist::RegisterOptions(void) {
	if (!llWorker::RegisterOptions()) return 0;

	RegisterValue("-name",  &mapname);
	RegisterValue("-water", &watername);

	RegisterFlag ("-center", &opt_center);
	RegisterValue("-dimX",   &opt_size_x);
	RegisterValue("-dimY",   &opt_size_y);

	RegisterValue("-quadoffsetx", &quadoffsetx);
	RegisterValue("-quadoffsety", &quadoffsety);
	RegisterFlag ("-autooffset",  &autooffset);

	RegisterValue("-x1", &x1);
	RegisterValue("-y1", &y1);
	RegisterValue("-x2", &x2);
	RegisterValue("-y2", &y2);

	RegisterFlag ("-readwaterheight", &readwaterheight);

	return 1;
}

int llImportMapFromModlist::Exec(void) {
	if (!llWorker::Exec()) return 0;

	if (!Used("-name"))  mapname   = "_heightmap";
	if (!Used("-water")) watername = "_watermap";

	if (_llMapList()->GetMap(mapname)) {
		_llLogger()->WriteNextLine(-LOG_INFO, "Delete old heightmap");
		_llMapList()->DeleteMap(mapname);
	}

	if (tes4qlod) delete tes4qlod;
	tes4qlod = new TES4qLOD();

	float waterdefaultheight = 0;
	if (_llUtils()->GetValue("_waterdefaultheight")) 
		waterdefaultheight = *_llUtils()->GetValueF("_waterdefaultheight");
	tes4qlod->waterheight = waterdefaultheight;

	char dummy[100];

	tes4qlod->RegisterOptions();
	tes4qlod->CheckFlag("-x");
	tes4qlod->CheckFlag("-silent");
	if (Used("-center")) tes4qlod->CheckFlag("-z");
	if (Used("-dimX")) {
		sprintf_s(dummy, 100, "%i", opt_size_x);
		tes4qlod->SetValue("-dimX", dummy);
	}
	if (Used("-dimY")) {
		sprintf_s(dummy, 100, "%i", opt_size_y);
		tes4qlod->SetValue("-dimY", dummy);
	}
	tes4qlod->Prepare();
	tes4qlod->ReplaceFlags();
	tes4qlod->Print();

	if (!tes4qlod->Exec()) {
		tes4qlod = NULL;
		delete tes4qlod;
		return 0;
	}
	if (!Used("-x1")) 
		x1 = TES4qLOD::min_x*(*_llUtils()->GetValueF("_cellsize_x"));
	else
		TES4qLOD::min_x = x1 / (*_llUtils()->GetValueF("_cellsize_x"));
	if (!Used("-y1")) 
		y1 = TES4qLOD::min_y*(*_llUtils()->GetValueF("_cellsize_y"));
	else
		TES4qLOD::min_y = y1 / (*_llUtils()->GetValueF("_cellsize_y"));
	if (!Used("-x2")) 
		x2 = (TES4qLOD::max_x + 1)*(*_llUtils()->GetValueF("_cellsize_x"));
	else
		TES4qLOD::max_x = x2 / (*_llUtils()->GetValueF("_cellsize_x")) - 1;
	if (!Used("-y2")) 
		y2 = (TES4qLOD::max_y + 1)*(*_llUtils()->GetValueF("_cellsize_y"));
	else
		TES4qLOD::max_y = y2 / (*_llUtils()->GetValueF("_cellsize_y")) - 1;

	float defaultheight = 0;
	if (_llUtils()->GetValue("_defaultheight")) defaultheight = *_llUtils()->GetValueF("_defaultheight");
	llMap *heightmap = new llMap((TES4qLOD::max_x - TES4qLOD::min_x + 1)*32+1, (TES4qLOD::max_y - TES4qLOD::min_y + 1)*32+1, 0, defaultheight/8.0f);
	heightmap->SetCoordSystem(x1, y1, x2, y2, 8.0f);

	llMap *watermap = NULL;
	if (readwaterheight) {
		watermap = new llMap(TES4qLOD::max_x - TES4qLOD::min_x + 1, TES4qLOD::max_y - TES4qLOD::min_y + 1, 0, tes4qlod->waterheight);
	} else {
		watermap = new llMap(TES4qLOD::max_x - TES4qLOD::min_x + 1, TES4qLOD::max_y - TES4qLOD::min_y + 1, 0, waterdefaultheight);
	}
	watermap->SetEven();
	watermap->SetCoordSystem(x1, y1, x2, y2, 1.0f);

	_llUtils()->x00 = x1;
	_llUtils()->y00 = y1;
	_llUtils()->x11 = x2;
	_llUtils()->y11 = y2;

	if (autooffset) {
		quadoffsetx = -x1;
		quadoffsety = -y1;
	}

	llQuadList     *quads      = heightmap->GenerateQuadList(quadoffsetx, quadoffsety);
	llPointList    *points     = new llPointList(0, quads); 
	llPolygonList  *polygons   = new llPolygonList(points, heightmap);
	llLineList     *lines      = new llLineList(0, points, heightmap);
	llTriangleList *triangles  = new llTriangleList(0, points);

	llQuadList     *wquads      = watermap->GenerateQuadList(quadoffsetx, quadoffsety);
	llPointList    *wpoints     = new llPointList(0, wquads); 
	llPolygonList  *wpolygons   = new llPolygonList(wpoints, watermap);
	llLineList     *wlines      = new llLineList(0, wpoints, watermap);
	llTriangleList *wtriangles  = new llTriangleList(0, wpoints);
		
	_llMapList()->AddMap(mapname,   heightmap, points,  triangles,  polygons, lines);
	_llMapList()->AddMap(watername, watermap,  wpoints, wtriangles, wpolygons, wlines);

	delete tes4qlod;
	tes4qlod = new TES4qLOD();

	tes4qlod->x_cell = TES4qLOD::min_x;
	tes4qlod->y_cell = TES4qLOD::min_y;
	_llLogger()->WriteNextLine(-LOG_INFO, "x corner: %i", tes4qlod->x_cell);
	_llLogger()->WriteNextLine(-LOG_INFO, "y corner: %i", tes4qlod->y_cell);

	tes4qlod->RegisterOptions();
	tes4qlod->CheckFlag("-map=_heightmap");
	tes4qlod->CheckFlag("-watermap=_watermap");
	tes4qlod->CheckFlag("-silent");
	tes4qlod->CheckFlag("-M");
	tes4qlod->CheckFlag("-AddKeepout");
	if (Used("-center")) tes4qlod->CheckFlag("-z");
	if (Used("-dimX")) {
		sprintf_s(dummy, 100, "%i", opt_size_x);
		tes4qlod->SetValue("-dimX", dummy);
	}
	if (Used("-dimY")) {
		sprintf_s(dummy, 100, "%i", opt_size_y);
		tes4qlod->SetValue("-dimY", dummy);
	}
	if (Used("-x1")) {
		sprintf_s(dummy, 100, "%i", tes4qlod->x_cell);
		tes4qlod->SetValue("-x1", dummy);
		if (Used("-x2")) {
			sprintf_s(dummy, 100, "%i", TES4qLOD::max_x - TES4qLOD::min_x + 1);
			tes4qlod->SetValue("-dimX", dummy);
		}
	}
	if (Used("-y1")) {
		sprintf_s(dummy, 100, "%i", tes4qlod->y_cell);
		tes4qlod->SetValue("-y1", dummy);
		if (Used("-y2")) {
			sprintf_s(dummy, 100, "%i", TES4qLOD::max_y - TES4qLOD::min_y + 1);
			tes4qlod->SetValue("-dimY", dummy);
		}
	}
	tes4qlod->Prepare();
	tes4qlod->ReplaceFlags();

	if (!tes4qlod->Exec()) {
		delete tes4qlod;
		tes4qlod = NULL;
		return 0;
	}

	delete tes4qlod;
	tes4qlod = NULL;

	return 1;
}
