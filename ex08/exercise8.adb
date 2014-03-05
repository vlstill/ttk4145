with Ada.Text_IO, Ada.Integer_Text_IO, Ada.Numerics.Float_Random;
use  Ada.Text_IO, Ada.Integer_Text_IO, Ada.Numerics.Float_Random;

-- (Ada tabs = 3 spaces)

procedure exercise8 is

   Count_Failed   : exception;   -- Exception to be raised when counting fails
   Gen            : Generator;   -- Random number generator

   protected type Transaction_Manager (N : Positive) is
      entry       Finished;
	 	  entry 			Wait_Until_Aborted;
      procedure   Signal_Abort;
   private
      Finished_Gate_Open   : Boolean := False;
      Aborted              : Boolean := False;
   end Transaction_Manager;
   protected body Transaction_Manager is
      entry Finished when Finished_Gate_Open or Finished'Count = N is
      begin
         Put_Line( "Manager.Finished: count =" & Integer'Image( Finished'Count )
                    & " aborted = " & Boolean'Image( Aborted ) );
				 if Finished'Count = N - 1 then
					 	Finished_Gate_Open := True;
				 end if;

         if Finished'Count = 0 then
            Finished_Gate_Open := False;
         end if;
      end Finished;

			entry Wait_Until_Aborted when Aborted is
			begin
				 Put_Line( "Wait_Until_Aborted triggered, count =" & Integer'Image( Wait_Until_Aborted'Count ) );
				 if Wait_Until_Aborted'Count = 0 then
					  Aborted := False;
				 end if;
			end Wait_Until_Aborted;

      procedure Signal_Abort is
      begin
         Aborted := True;
      end Signal_Abort;
      
  end Transaction_Manager;



   
   function Unreliable_Slow_Add (x : Integer) return Integer is
   Error_Rate : Constant := 0.15;  -- (between 0 and 1)
   begin
      if Random( Gen ) <= Error_Rate then
         delay Duration( Random( Gen ) * 0.5 );
         raise Count_Failed;
      else
         delay Duration( Random( Gen ) * 4.0 );
         return 10 + x;
      end if;
   end Unreliable_Slow_Add;




   task type Transaction_Worker (Initial : Integer; Manager : access Transaction_Manager);
   task body Transaction_Worker is
      Num         : Integer   := Initial;
      Prev        : Integer   := Num;
      Round_Num   : Integer   := 0;
   begin
      Put_Line ("Worker" & Integer'Image(Initial) & " started");

      loop
         Put_Line ("Worker" & Integer'Image(Initial) & " started round" & Integer'Image(Round_Num));
         Round_Num := Round_Num + 1;

				 select
				 		Manager.Wait_Until_Aborted;
				 		Num := Prev + 5;
				 		Put_Line( "  Worker" & Integer'Image( Initial ) & ": forward recovery triggered,"
											& "changing from " & Integer'Image( Prev ) & " to " & Integer'Image( Num ) );
				 then abort
						begin
							 Num := Unreliable_Slow_Add( Num );
							 Manager.Finished;
							 Put_Line ("  Worker" & Integer'Image(Initial) & " comitting" & Integer'Image(Num));
						exception
							 when Count_Failed => 
								  begin
										 Put_Line( "  Worker" & Integer'Image( Initial ) & " aborting" );
										 Manager.Signal_Abort;
									end;
						end;
				 end select;

         Prev := Num;
         delay 3.0;

      end loop;
   end Transaction_Worker;

   Manager : aliased Transaction_Manager (3);

   Worker_1 : Transaction_Worker (0, Manager'Access);
   Worker_2 : Transaction_Worker (1, Manager'Access);
   Worker_3 : Transaction_Worker (2, Manager'Access);

begin
   Reset(Gen); -- Seed the random number generator
end exercise8;



