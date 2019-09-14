function vxtimercb(gcb)
% VXTIMERCB Mask parameter callback used by VxWorks Interrupt and Task blocks
% Copyright 2005 The MathWorks, Inc.
% $Revision $

switch get_param(gcb,'MaskType')
  case 'VxWorks Interrupt Block'
   if length(get_param(gcbh,'MaskCallbacks')) == 8
     a = get_param(gcb,'MaskValues');
     if strcmp(a{5},'on')
       set_param(gcb,'MaskVisibilities',{'on','on','on','on','on','on','on','on'});
     else
       set_param(gcb,'MaskVisibilities',{'on','on','on','on','on','off','off','on'});
     end
  else
    errMsg = sprintf('Mask parameter callback for block ''%s'' expected 8 mask parameters.', ...
                get_param(gcb,'Name'));
    warning(errMsg);
  end
     
 case 'VxWorks Task Block'
  if length(get_param(gcbh,'MaskCallbacks')) == 6
    a=get_param(gcb,'MaskValues');
    if strcmp(a{4},'on'),
      set_param(gcb,'MaskVisibilities',{'on','on','on','on','off','off'});
    else,
      set_param(gcb,'MaskVisibilities',{'on','on','on','on','on','on'});
    end
  else
    errMsg = sprintf('Mask parameter callback for block ''%s'' expected 6 mask parameters.', ...
                get_param(gcb,'Name'));
    warning(errMsg);
  end
 
 otherwise
  errMsg = sprintf('Mask parameter callback function called with unexpected MaskType ''%s''.', ...
                   get_param(gcb,'MaskType'));
  warning(errMsg);
end

  