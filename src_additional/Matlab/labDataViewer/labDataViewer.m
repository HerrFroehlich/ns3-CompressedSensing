function varargout = labDataViewer(varargin)
% LABDATAVIEWER MATLAB code for labDataViewer.fig
%      LABDATAVIEWER, by itself, creates a new LABDATAVIEWER or raises the existing
%      singleton*.
%
%      H = LABDATAVIEWER returns the handle to a new LABDATAVIEWER or the handle to
%      the existing singleton*.
%
%      LABDATAVIEWER('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in LABDATAVIEWER.M with the given input arguments.
%
%      LABDATAVIEWER('Property','Value',...) creates a new LABDATAVIEWER or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before labDataViewer_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to labDataViewer_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help labDataViewer

% Last Modified by GUIDE v2.5 12-Jul-2017 07:18:19

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @labDataViewer_OpeningFcn, ...
                   'gui_OutputFcn',  @labDataViewer_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before labDataViewer is made visible.
function labDataViewer_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to labDataViewer (see VARARGIN)

% Choose default command line output for labDataViewer
handles.output = hObject;
% default values
handles.dir = '';
handles.NOF_NODES = 58;
handles.NOF_FIELDS = 8;
handles.id = 1;
handles.filename = 'temp';
handles.type = 'double';
handles.data = [];
handles.epoch = [];
handles.dataSize = 0;
% Update handles structure
guidata(hObject, handles);

% UIWAIT makes labDataViewer wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = labDataViewer_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes on button press in pushbutton1.
function pushbutton1_Callback(hObject, eventdata, handles)
dir = uigetdir('./');
if(dir ~= 0)
    handles.dir = dir;
    % Update handles structure
    handles = ReadFile(handles);
    Replot(handles);
    guidata(hObject, handles);
end



function edit1_Callback(hObject, eventdata, handles)

% --- Executes during object creation, after setting all properties.
function edit1_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on selection change in popupmenu1.
function popupmenu1_Callback(hObject, eventdata, handles)
filename = '';
type = 'double';
switch get(handles.popupmenu1,'Value')
    case 1
        filename = 'temp';
    case 2
        filename = 'hum';
    case 3
        filename = 'light';
    case 4
        filename = 'volt';
    case 5
        filename = 'epoch';
        type = 'uint16';
end
handles.filename = filename;
handles.type = type;
handles = ReadFile(handles);
Replot(handles);
guidata(hObject, handles);

% --- Executes during object creation, after setting all properties.
function popupmenu1_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes during object creation, after setting all properties.
function axes1_CreateFcn(hObject, eventdata, handles)


% --- Executes on selection change in idpopupmenu.
function idpopupmenu_Callback(hObject, eventdata, handles)
% hObject    handle to idpopupmenu (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: contents = cellstr(get(hObject,'String')) returns idpopupmenu contents as cell array
%        contents{get(hObject,'Value')} returns selected item from idpopupmenu
contents = cellstr(get(hObject,'String'));
handles.id = contents{get(hObject,'Value')};

handles = ReadFile(handles);
Replot(handles);
% Update handles structure
guidata(hObject, handles);

% --- Executes during object creation, after setting all properties.
function idpopupmenu_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end
text = cell(58);
for i = 1:58
    text{i} = num2str(i);
end
set(hObject, 'String', text);

function handles = ReadFile(handles)

filename = handles.filename;
id = handles.id;
path = [handles.dir '/' num2str(id) '/' filename];
fid = fopen(path);
handles.data = fread(fid, handles.type);
fclose(fid);
handles.dataSize = numel(handles.data);
%read epoch
path = [handles.dir '/' num2str(id) '/epoch'];
fid = fopen(path);
handles.epoch = fread(fid, 'uint16');
fclose(fid);
% set slider
handles.slider1.Max = handles.dataSize;
handles.slider1.Value = handles.dataSize;

function Replot(handles)

max = uint32(handles.slider1.Value);
if handles.togglebutton1.Value == 1
    plot(handles.axes1, handles.epoch(1:max), handles.data(1:max),'.','MarkerSize',1);
%     hold(handles.axes1,'on');
%     diff = setdiff(handles.epoch(1:max)',1:max);
%     val = zeros(1, numel(diff));
%     stem(handles.axes1, diff, val, 'rx');
%     
%     hold(handles.axes1,'off');
else
    plot(handles.axes1, handles.data(1:max));
end


% --- Executes on button press in togglebutton1.
function togglebutton1_Callback(hObject, eventdata, handles)
% hObject    handle to togglebutton1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of togglebutton1
Replot(handles);

% --- Executes on slider movement.
function slider1_Callback(hObject, eventdata, handles)
% hObject    handle to slider1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
Replot(handles);

% --- Executes during object creation, after setting all properties.
function slider1_CreateFcn(hObject, eventdata, handles)
% hObject    handle to slider1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end
