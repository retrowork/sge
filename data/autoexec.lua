-------------------------------------------------------------------------------
-- $Id$

function QuitWithConfirm()
   ConfirmedQuit("quitdlg.xml");
end;

bind("q", "quit();");
bind("escape", [[QuitWithConfirm();]]);

bind("F4", [[GUIContext:ToggleDebugInfo(5, 200, "yellow");]]);

--LogChannel([[LuaInterp]]);
--LogChannel([[GUIButtonEvents]]);
--LogChannel([[GUIDialogEvents]]);

gameMeshes = 
{
   "tree2.ms3d",
   "crate.ms3d",
   "face.3ds",
   "zombie.ms3d",
};

function LoadSampleLevel()
   GUIContext:Clear();
   GUIContext:Load("guitest.xml");

   SetTerrain("ground.tga", 0.2, "grass.tga");

   nGameMeshes = table.getn(gameMeshes);
   for i = 1, nGameMeshes do
	  for m=1,5 do
	     EntitySpawnTest(gameMeshes[i], math.random(), math.random());
	  end
   end
   
   EntitySpawnTest("zombie.ms3d", 0.5, 0.4);
   ViewSetPos(0.5, 0.4);
end;

-- Called automatically at start-up by the game engine
function GameInit()
   GUIContext:Clear();
   GUIContext:Load("start.xml");
end;

-------------------------------------------------------------------------------
