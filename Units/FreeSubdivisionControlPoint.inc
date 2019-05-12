{--------------------------------------------------------------------------------------------------}
{                                         TFreeSubdivisionControlPoint                             }
{--------------------------------------------------------------------------------------------------}
function TFreeSubdivisionControlPoint.FGetColor: TColor;
begin
  if Selected then
    Result := Owner.Selectedcolor
  else
  begin
    if Locked then
      Result := clDkGray
    else
    begin
      if IsLeak then
        Result := Owner.LeakColor
      else
        case FVertexType of
          svRegular: Result := Owner.RegularPointColor;
          svCorner: Result := Owner.CornerPointColor;
          svDart: Result := Owner.DartPointColor;
          svCrease: Result := Owner.CreasePointColor;
          else
            Result := clRed;
        end;
    end;
  end;
end;{TFreeSubdivisionControlPoint.FGetColor}

function TFreeSubdivisionControlPoint.FGetIndex: integer;
begin
  Result := Owner.FControlPoints.IndexOf(Self);
end;{TFreeSubdivisionControlPoint.FGetIndex}

function TFreeSubdivisionControlPoint.FGetIsLeak: boolean;
begin
  Result := (abs(Coordinate.Y) > 1e-4) and (IsBoundaryVertex);
end;{TFreeSubdivisionControlPoint.FGetIsLeak}

function TFreeSubdivisionControlPoint.FGetSelected: boolean;
begin
  Result := Owner.FSelectedControlPoints.IndexOf(self) <> -1;
end;{TFreeSubdivisionControlPoint.FGetSelected}

function TFreeSubdivisionControlPoint.FGetVisible: boolean;
var
  I: integer;
  CFace: TFreeSubdivisionControlFace;
begin
  // meant for controlpoints only.
  // a controlpoint is visible if at least one of it's
  // neighbouring controlfaces belongs to a visible layer
  Result := False;
  if Owner.ShowControlNet then
  begin
    for i := 0 to FFaces.Count-1 do
      if FFaces[I] is TFreeSubdivisionControlFace then
      begin
        CFace := TFreeSubdivisionControlFace(FFaces[I]);
        if CFace.Layer <> nil then
        begin
          if CFace.Layer.Visible then
          begin
            Result := True;
            exit;
          end;
        end;
      end;
  end;
  // Finally check if the point is selected.
  // Selected points must be visible at all times
  // Points with no faces connected also!
  if not Result then
    Result := (Selected) or (NumberOfFaces = 0);
  if (not Result) and (Owner.NumberOfControlCurves > 0) then
  begin
    for I := 1 to NumberOfEdges do
      if Edge[I - 1].FCurve <> nil then
      begin
        if Edge[I - 1].FCurve.Selected then
        begin
          Result := True;
          break;
        end;
      end;
  end;
end;{TFreeSubdivisionControlPoint.FGetVisible}

procedure TFreeSubdivisionControlPoint.FSetSelected(val: boolean);
var
  Index: integer;
begin
  Index := Owner.FSelectedControlPoints.IndexOf(self);
  if Val then
  begin
    // Only add if it is not already in the list
    if Index = -1 then
      Owner.FSelectedControlPoints.Add(self);
  end
  else
  begin
    if Index <> -1 then
      Owner.FSelectedControlPoints.Delete(index);
  end;
  if Assigned(Owner.FOnSelectItem) then
    Owner.FOnSelectItem(self);
end;{TFreeSubdivisionControlPoint.FSetSelected}

procedure TFreeSubdivisionControlPoint.FSetLocked(val: boolean);
begin
  if Val <> FLocked then
  begin
    FLocked := Val;
  end;
end;{TFreeSubdivisionControlPoint.FSetLocked}

procedure TFreeSubdivisionControlPoint.FSetCoordinate(Val: T3DCoordinate);
begin
  if not Locked then
  begin
    inherited FSetCoordinate(Val);
    Owner.Build := False;
  end;
end;{TFreeSubdivisionControlPoint.FSetCoordinate}

procedure TFreeSubdivisionControlPoint.Collapse;
var
  I, J: integer;
  Face: TFreeSubdivisionControlFace;
  Edge1: TFreesubdivisionControlEdge;
  Edge2: TFreesubdivisionControlEdge;
  Edges: TFasterListTFreeSubdivisionControlEdge;
  Points: TFasterListTFreeSubdivisionPoint;
  SortedPointLists: TFasterListTFreeSubdivisionFace; // using Face class as list of Points
  sPointList:TFreeSubdivisionFace;
  Checklist: TFasterListTFreeSubdivisionControlPoint;
  P1, P2: TFreeSubdivisionControlPoint;
  Crease: boolean;
  EdgeCollapse: boolean;
begin
  if NumberOfFaces <= 2 then
  begin
    Selected := False;
    P1 := nil;
    P2 := nil;
    // This is possibly a point on a boundary edge,
    // check for this special case
    Edge1 := nil;
    Edge2 := nil;
    for i := 1 to NumberOfEdges do
      if Edge[I - 1].NumberOfFaces = 1 then
      begin
        if Edge1 = nil then
          Edge1 := Edge[I - 1] as TFreesubdivisionControlEdge
        else
          Edge2 := Edge[I - 1] as TFreesubdivisionControlEdge;
      end;
    if (Edge1 <> nil) and (Edge2 <> nil) then
    begin
      for I := NumberOfEdges downto 1 do
        if Edge[I - 1].NumberOfFaces > 1 then
        begin
          Edge1 := Edge[I - 1] as TFreesubdivisionControlEdge;
          Edge1.Collapse;
        end;
    end;

    if NumberOfEdges = 2 then
    begin
      EdgeCollapse := True;
      Edge1 := FEdges[0] as TFreesubdivisionControlEdge;
      if Edge1.FStartpoint = self then
        P1 := Edge1.FEndpoint as TFreeSubdivisionControlPoint
      else
        P1 := Edge1.FStartpoint as TFreeSubdivisionControlPoint;
      Edge2 := FEdges[1] as TFreesubdivisionControlEdge;
      if Edge2.FStartpoint = self then
        P2 := Edge2.FEndpoint as TFreeSubdivisionControlPoint
      else
        P2 := Edge2.FStartpoint as TFreeSubdivisionControlPoint;
      Crease := Edge1.Crease or Edge2.Crease;
    end
    else
    begin
      Crease := False;
      EdgeCollapse := False;
    end;
    for I := NumberOfFaces downto 1 do
    begin
      Face := FFaces[I - 1] as TFreeSubdivisionControlFace;
      Points := TFasterListTFreeSubdivisionPoint.Create;
      for J := 1 to Face.FPoints.Count do
        if Face.FPoints[J - 1] <> self then
          Points.Add(Face.FPoints[J - 1]);
      FOwner.AddControlFace(TFasterList(Points), False, Face.Layer);
      Points.Destroy;
      Face.Delete;
    end;

    if EdgeCollapse then
    begin
      Edge1 := FOwner.EdgeExists(P1, P2) as TFreesubdivisionControlEdge;
      if Edge1 <> nil then
      begin
        Edge1.Crease := Crease;// or (Edge1.NumberOfFaces=1);
      end;
    end;

  end
  else
  begin
    Checklist := TFasterListTFreeSubdivisionControlPoint.Create;
    Edges := TFasterListTFreeSubdivisionControlEdge.Create;
    for i := 1 to NumberOfEdges do
      if Edge[I - 1].StartPoint = self then
        CheckList.Add(Edge[I - 1].EndPoint as TFreeSubdivisionControlPoint)
      else
        CheckList.Add(Edge[I - 1].StartPoint as TFreeSubdivisionControlPoint);
    for I := 1 to NumberOfFaces do
    begin
      Face := Ffaces[I - 1] as TFreeSubdivisionControlFace;
      P1 := Face.FPoints[Face.NumberOfPoints - 1] as TFreeSubdivisionControlPoint;
      for J := 1 to Face.FPoints.Count do
      begin
        P2 := face.FPoints[J - 1] as TFreeSubdivisionControlPoint;
        if (P1 <> self) and (P2 <> self) then
        begin
          Edge1 := FOwner.EdgeExists(P1, P2) as TFreesubdivisionControlEdge;
          if Edge1 <> nil then
            if Edges.IndexOf(Edge1) = -1 then
              Edges.Add(Edge1);
        end;
        P1 := P2;
      end;
    end;
    // sort edges in correct order and add new face
    if Edges.Count > 2 then
    begin
      SortedPointLists := TFasterListTFreeSubdivisionFace.Create;
      Fowner.IsolateEdges(TFasterList(Edges), SortedPointLists);
      Edges.Destroy;
      for I := 1 to SortedPointLists.Count do
      begin
        // was Points := Sorted[I - 1]; ??? Points := ControlEdges?
        sPointList := SortedPointLists[I - 1];
        Points:= sPointList.FPoints;
        if Points.Count > 2 then
        begin
          Face := FOwner.AddControlFace(TFasterList(Points), False);
          if Face <> nil then
          begin
            P1 := Face.FPoints[Face.FPoints.Count - 1] as TFreeSubdivisionControlPoint;
            for J := 1 to Face.FPoints.Count do
            begin
              P2 := face.FPoints[J - 1] as TFreeSubdivisionControlPoint;
              //                     Edge1:=fowner.EdgeExists(p1,p2) as TFreesubdivisionControlEdge;
              //                     if edge1<>nil then
              //                     begin
              //                        {
              //                        Cur:=Edge.FFaces.IndexOf(self);
              //                        if Cur=-1 then Edge.Crease:=Edge.NumberOfFaces<2 else
              //                           Edge.Crease:=Edge.NumberOfFaces<3;
              //                        }
              //                     end;
              P1 := P2;
            end;
          end;
        end;
        Points.Free;
      end;
      SortedPointLists.Free;
    end
    else
      Edges.Destroy;
    Delete;

    for i := Checklist.Count downto 1 do
    begin
      P1 := Checklist[I - 1];
      if (P1.NumberOfFaces > 1) and (P1.NumberOfEdges = 2) then
      begin
        P1.Collapse;
      end;
    end;
    checklist.Destroy;
  end;
end;{TFreeSubdivisionControlPoint.Collapse}

constructor TFreeSubdivisionControlPoint.Create(Owner: TFreeSubdivisionSurface);
begin
  inherited Create(Owner);
end;{TFreeSubdivisionControlPoint.Create}

procedure TFreeSubdivisionControlPoint.Delete;
var
  Index: integer;
  Edge: TFreesubdivisionControlEdge;
begin
  // delete from selection;
  Selected := False;
  Dereference;
  if FEdges.Count > 0 then
  begin
    Index := Owner.FControlPoints.IndexOf(Self);
    if Index <> -1 then
      Owner.FControlPoints.Delete(index);

    while FEdges <> nil do //.Count>0 do
    begin
      Edge := self.Edge[NumberOfEdges - 1] as TFreesubdivisionControlEdge;
      Edge.Delete;
    end;
  end
  else
  begin
    Index := Owner.FControlPoints.IndexOf(Self);
    if Index <> -1 then
      Owner.FControlPoints.Delete(index);
    Destroy;
  end;
end;{TFreeSubdivisionControlPoint.Delete}

procedure TFreeSubdivisionControlPoint.Draw(Viewport: TFreeViewport);
var
  P: TPoint;
  Pz: TShadePoint;
  I: integer;
  P3D: T3DCoordinate;
begin
  if Viewport.ViewportMode <> vmWireframe then
  begin
    P3D := FCoordinate;
    if Viewport.ViewType = fvBodyplan then
      if P3D.X <= Owner.MainframeLocation then
        P3D.Y := -P3D.Y;
    P := Viewport.Project(P3D);
    Pz := Viewport.ProjectToZBuffer(1.002 * ZBufferScaleFactor, P3D);
    // Check if the point lies within the viewport's drawingcanvas boundaries
    if (Pz.X >= 0) and (Pz.Y >= 0) and (Pz.X < Viewport.ClientWidth) and
      (Pz.Y < Viewport.ClientHeight) then
    begin
      // Compare to Z buffer to check visibility;
      if Pz.Z >= Viewport.ZBuffer.FBuffer[Pz.Y][Pz.X] then
      begin
        // yes, the point is visible
        Viewport.ZBuffer.FBuffer[Pz.Y][Pz.X] := Pz.Z;
        if Viewport.PenWidth <> 1 then
          Viewport.PenWidth := 1;
        Viewport.PenColor := Color;
        Viewport.BrushStyle := bsClear;
        Viewport.PenStyle := psSolid;
        for I := 1 to Owner.ControlPointSize do
          Viewport.Rectangle(Pz.X - I, Pz.Y - I, Pz.X + I, Pz.Y + I);
      end;
    end;
  end
  else
  begin
    //with Viewport.DrawingCanvas do
    //begin
    P3D := FCoordinate;
    if Viewport.ViewType = fvBodyplan then
      if P3D.X <= Owner.MainframeLocation then
        P3D.Y := -P3D.Y;
    P := Viewport.Project(P3D);
    if Viewport.PenWidth <> 1 then
      Viewport.PenWidth := 1;
    Viewport.PenColor := Color;
    if Viewport.BrushStyle <> bsClear then
      Viewport.BrushStyle := bsClear;
    if Selected then
    begin
      //FInitializeCanvas(Viewport,1,Color,Mode);
      for I := 1 to Owner.ControlPointSize do
        Viewport.Rectangle(P.X - I, P.Y - I, P.X + I, P.Y + I);
      Viewport.Rectangle(P.X - Owner.ControlPointSize - 2, P.Y -
        Owner.ControlPointSize - 2, P.X + Owner.ControlPointSize + 2, P.Y + Owner.ControlPointSize + 2);
    end
    else
    begin
      for I := 1 to Owner.ControlPointSize do
        Viewport.Rectangle(P.X - I, P.Y - I, P.X + I, P.Y + I);
    end;
    //end;
  end;
end;{TFreeSubdivisionControlPoint.Draw}

function TFreeSubdivisionControlPoint.DistanceToCursor(X, Y: integer;
  Viewport: TFreeViewport): integer;
var
  Pt: TPoint;
  P: T3DCoordinate;
begin
  // Check if cursor position lies within the boundaries
  P := FCoordinate;
  if (Viewport.ViewType = fvBodyplan) and (P.X <= Owner.MainframeLocation) then
    P.Y := -P.Y;
  Pt := Viewport.Project(P);
  if (Pt.X >= 0) and (Pt.X <= Viewport.Width) and (Pt.Y >= 0) and (Pt.Y <= Viewport.Height) then
    Result := Round(Sqrt(Sqr(Pt.X - X) + SQR(Pt.Y - Y)))
  else
    Result := 1000000;
end;{TFreeSubdivisionControlPoint.DistanceToCursor}

procedure TFreeSubdivisionControlPoint.LoadBinary(Source: TFreeFileBuffer);
var
  I: integer;
  Sel: boolean;
begin
  Source.Load(FCoordinate);
  Source.Load(I);
  FVertextype := TFreeVertexType(I);
  Source.Load(Sel);
  if Sel then
    Selected := True;
  if Source.Version >= fv198 then
  begin
    Source.Load(FLocked);
  end;
end;{TFreeSubdivisionControlPoint.LoadBinary}

procedure TFreeSubdivisionControlPoint.LoadFromStream(
  var LineNr: integer; Strings: TStringList);
var
  Str: string;
  I: integer;
  sel: boolean;
begin
  // FCoordinate
  Inc(LineNr);
  Str := Strings[LineNr];
  FCoordinate.X := ReadFloatFromStr(LineNr, Str);
  FCoordinate.Y := ReadFloatFromStr(LineNr, Str);
  FCoordinate.Z := ReadFloatFromStr(LineNr, Str);
  // FVertexType
  if Str <> '' then
  begin
    I := ReadIntFromStr(LineNr, Str);
    FVertextype := TFreeVertexType(I);
    if Str <> '' then
    begin
      Sel := ReadBoolFromStr(LineNr, Str);
      if sel then
        Selected := True;
    end;
  end
  else
    FVertextype := TFreeVertexType(0);
end;{TFreeSubdivisionControlPoint.LoadFromStream}

procedure TFreeSubdivisionControlPoint.SaveBinary(Destination: TFreeFileBuffer);
begin
  Destination.Add(FCoordinate);
  Destination.Add(Ord(VertexType));
  Destination.Add(Selected);
  if Destination.Version >= fv198 then
  begin
    Destination.Add(Locked);
  end;
end;{TFreeSubdivisionControlPoint.SaveBinary}

procedure TFreeSubdivisionControlPoint.SaveToStream(Strings: TStringList);
var
  Str: string;
begin
  Str := Truncate(Fcoordinate.X, 5) + #32 + Truncate(Fcoordinate.Y, 5) + #32 +
    Truncate(Fcoordinate.Z, 5);
  Str := Str + #32 + IntToStr(Ord(VertexType));
  Str := Str + #32 + BoolToStr(Selected);
  Strings.Add(Str);
end;{TFreeSubdivisionControlPoint.SaveToStream}
