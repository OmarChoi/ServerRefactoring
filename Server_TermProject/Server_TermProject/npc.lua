myid = 99999;
move_count = 0;

function set_info(x, type)
    myid = x;
    -- id, type, x, y, hp, level, 
    API_SetInfo(myid);
end

function Event_Get_Hit(player)
   player_x = API_get_player_x(player);
   player_y = API_get_player_y(player);
   my_x = API_get_npc_x(myid);
   my_y = API_get_npc_y(myid);
   if (math.abs(player_x - my_x) + math.abs(player_y - my_y) > 1 then
      if (player_y == my_y) then
         API_MoveStart(myid);
         API_SendMessage(myid, player, "HELLO");
      end
   end
end

function event_npc_move_away(player)
    move_count = move_count + 1;
    if(move_count > 2) then
        move_count = 0
		API_SendMessage(myid, player, "BYE")
    else
		API_NPCMove(myid, 1)
	end
end