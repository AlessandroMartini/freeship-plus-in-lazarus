unit FileIconGtk;
{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Graphics,
  FileIcon,
  GTK2, GLib2,
  FileIconGtk3;

type TFileIconGtk = class(TFileIconAdapter)
protected
  function getIconByNameGtk2(icon_name:Pgchar; size:integer):TIcon;
  function getIconByName( iconName:string; size:integer):TIcon;
  function getIconNameForFile(filename:string; size:integer):string; override;
  function getIconForFile(filename:string; size:integer):TIcon; override;
public
  function addIconsForFile(filename:string):integer; override;
end;

implementation

function TFileIconGtk.getIconByNameGtk2(icon_name:Pgchar; size:integer):TIcon;
var icon_theme: PGtkIconTheme;
    icon_info: PGtkIconInfo;
    icon_filename: Pgchar;
    iconFileName: string;
begin
  result := nil;
  icon_theme := gtk_icon_theme_get_default;
  if gtk_icon_theme_has_icon(icon_theme, icon_name) then
  begin
       icon_info := gtk_icon_theme_lookup_icon( icon_theme, icon_name, size,
                                                GTK_ICON_LOOKUP_NO_SVG);
       icon_filename := gtk_icon_info_get_filename(icon_info);
       iconFileName := String(icon_filename);
       result := LoadIconFromFile(iconFileName);
  end;
end;

function TFileIconGtk.getIconByName( iconName:string; size:integer):TIcon;
begin
  result := getIconByNameGtk2(PChar(iconName), size);
end;

function TFileIconGtk.getIconNameForFile(filename:string; size:integer):string;
var icon_theme: PGtkIconTheme;  icon_names:PPgchar; icon_name:Pgchar; i:integer;
begin
  result := '';
  icon_names := getIconNamesForFileGtk3(filename, size);
  icon_theme := gtk_icon_theme_get_default;
  for i:=0 to (sizeOf(icon_names) div sizeOf(Pgchar))-1 do
  begin
      icon_name := icon_names[i];
      if gtk_icon_theme_has_icon(icon_theme, icon_name) then
      begin
         result := String(icon_name);
         break;
      end;
  end;
end;

function TFileIconGtk.getIconForFile(filename:string; size:integer):TIcon;
var icon:TIcon;   icon_names:PPgchar; i:integer; icon_name:Pgchar;
begin
  result := nil;
    icon_names := getIconNamesForFileGtk3(filename, size);
    for i:=0 to sizeOf(icon_names) do
        begin
        icon_name := icon_names[i];
        icon := getIconByNameGtk2(icon_name, size);
        result := icon;
        if icon <> nil then break;
        end;

end;

// checks if icon is already cached and adds an icon if it is not cached,
// returns icon index in LargeIconList and SmallIconList
function TFileIconGtk.addIconsForFile(filename:string):integer;
var //mimeType:string;
    iconName:string; i,i1,i2,iconIndex:integer;
    smallIcon, largeIcon: TIcon;
begin
  result := -1;
  iconName := getIconNameForFile(filename,16);

  // check if icon is registered in iconName to IconIndex map
  i := FIconNamesMap.IndexOf(iconName);
  if (i>-1) then
    begin
    iconIndex := PtrUInt(FIconNamesMap.Objects[i]);
    result := iconIndex;
    end
  else
    begin
    iconIndex := -1;
    smallIcon := getIconByName(iconName,16);
    largeIcon := getIconByName(iconName,32);
    if assigned(smallIcon) and assigned(largeIcon) then
      begin
      i1 := FSmallImageList.AddIcon(smallIcon);
      i2 := FLargeImageList.AddIcon(largeIcon);
      iconIndex := i1;
      FIconNamesMap.AddObject(iconName, TObject(PtrUint(iconIndex)));
      result := iconIndex;
      end;
    end;
end;


end.

