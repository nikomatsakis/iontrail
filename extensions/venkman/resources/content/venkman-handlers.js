/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is The JavaScript Debugger
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

/* Alphabetical order, please. */

console.onLoad =
function con_load (e)
{
    dd ("Application venkman, 'JavaScript Debugger' loaded.");

    init();
    
}

console.onInputCommand =
function con_icommand (e)
{
    
    var ary = console._commands.list (e.command);
    
    switch (ary.length)
    {            
        case 0:
            display (getMsg(MSN_ERR_NO_COMMAND, e.command), MT_ERROR);
            break;
            
        case 1:
            if (typeof console[ary[0].func] == "undefined")        
                display (getMsg(MSN_ERR_NOTIMPLEMENTED, ary[0].name), MT_ERROR);
            else
            {
                e.commandEntry = ary[0];
                console[ary[0].func](e)
            }
            break;
            
        default:
            var str = "";
            for (var i in ary)
                str += str ? ", " + ary[i].name : ary[i].name;
            display (getMsg (MSN_ERR_AMBIGCOMMAND, [e.command, ary.length, str]),
                     MT_ERROR);
    }

}

console.onInputCommands =
function cli_icommands (e)
{
    displayCommands (e.inputData);
    return true;
}

console.onInputCont =
function cli_icommands (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }

    console.jsds.exitNestedEventLoop();

    return true;
}
    
console.onInputCompleteLine =
function con_icline (e)
{
    if (console._inputHistory[0] != e.line)
        console._inputHistory.unshift (e.line);
    
    if (console._inputHistory.length > console.prefs["input.history.max"])
        console._inputHistory.pop();

    console._lastHistoryReferenced = -1;
    console._incompleteLine = "";

    if (e.line[0] == console.prefs["input.commandchar"])
    {   /* starts with a '/', look up the command */
        var ary = e.line.substr(1, e.line.length).match (/(\S+)? ?(.*)/);
        var command = ary[1];

        e.command = command;
        e.inputData =  ary[2] ? stringTrim(ary[2]) : "";
        e.line = e.line;
        console.onInputCommand (e);
    }
    else /* no command character */
        evalInDebuggerScope (e.line);

}

console.onInputEval =
function con_ieval (e)
{
    if (e.inputData)
        evalInTargetScope (e.inputData)
    return true;
}

console.onInputFrame =
function con_iframe (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }

    var idx = parseInt(e.inputData);
    
    if (idx >= 0)
    {
        console.currentFrameIndex = idx;
        displayFrame (console.frames[idx], idx);
    }
    else
        displayFrame (console.frames[console.currentFrameIndex]);
    
    return true;
}
            
console.onInputHelp =
function cli_ihelp (e)
{
    var ary = console._commands.list (e.inputData);
 
    if (ary.length == 0)
    {
        display (getMsg(MSN_ERR_NO_COMMAND, e.inputData), MT_ERROR);
        return false;
    }

    for (var i in ary)
    {        
        display (ary[i].name + " " + ary[i].usage, MT_USAGE);
        display (ary[i].help, MT_HELP);
    }

    return true;    
}

console.onInputScope =
function con_iscope (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }
    
    if (console.frames[console.currentFrameIndex].scope.propertyCount == 0)
        display (getMsg (MSN_NO_PROPERTIES, MSG_VAL_SCOPE + " 0"));
    else
        displayProperties (console.frames[console.currentFrameIndex].scope);
    
    return true;
}

console.onInputQuit =
function con_iquit (e)
{
    window.close();
}

console.onInputWhere =
function con_iwhere (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }
    
    displayCallStack();
    return true;
}

console.onSingleLineKeypress =
function con_slkeypress (e)
{
    switch (e.keyCode)
    {
        case 13:
            if (!e.target.value)
                return;
            
            ev = new Object();
            ev.keyEvent = e;
            ev.line = e.target.value;
            console.onInputCompleteLine (ev);
            e.target.value = "";
            break;

        case 38: /* up */
            if (console._lastHistoryReferenced <
                console._inputHistory.length - 1)
                e.target.value =
                    console._inputHistory[++console._lastHistoryReferenced];
            break;

        case 40: /* down */
            if (console._lastHistoryReferenced > 0)
                e.target.value =
                    console._inputHistory[--console._lastHistoryReferenced];
            else
            {
                console._lastHistoryReferenced = -1;
                e.target.value = console._incompleteLine;
            }
            
            break;
            
        case 9: /* tab */
            e.preventDefault();
            console.onTabCompleteRequest(e);
            break;       

        default:
            console._incompleteLine = e.target.value;
            break;
    }
}

console.onTabCompleteRequest =
function con_tabcomplete (e)
{
    var selStart = e.target.selectionStart;
    var selEnd   = e.target.selectionEnd;            
    var v        = e.target.value;
    var cmdchar  = console.prefs["input.commandchar"];
    
    if (selStart != selEnd) 
    {
        /* text is highlighted, just move caret to end and exit */
        e.target.selectionStart = e.target.selectionEnd = v.length;
        return;
    }

    var firstSpace = v.indexOf(" ");
    if (firstSpace == -1)
        firstSpace = v.length;

    var pfx;
    var d;
    
    if ((v[0] == cmdchar) && (selStart <= firstSpace))
    {
        /* line starts with /, and the cursor is positioned before the
         * first space, so we're completing a command
         */
        var partialCommand = v.substring(1, firstSpace).toLowerCase();
        var cmds = console._commands.listNames(partialCommand);

        if (!cmds)
            /* partial didn't match a thing */
            display (getMsg(MSN_NO_CMDMATCH, partialCommand), MT_ERROR);
        else if (cmds.length == 1)
        {
            /* partial matched exactly one command */
            pfx = cmdchar + cmds[0];
            if (firstSpace == v.length)
                v =  pfx + " ";
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length + 1;
        }
        else if (cmds.length > 1)
        {
            /* partial matched more than one command */
            d = new Date();
            if ((d - console._lastTabUp) <= console.prefs["input.dtab.time"])
                display (getMsg (MSN_CMDMATCH,
                                 [partialCommand, "[" + cmds.join(", ") + "]"]));
            else
                console._lastTabUp = d;
            
            pfx = cmdchar + getCommonPfx(cmds);
            if (firstSpace == v.length)
                v =  pfx;
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length;
            
        }
                
    }

}

console.onUnload =
function con_unload (e)
{
    dd ("Application venkman, 'JavaScript Debugger' unloaded.");

    detachDebugger();
}

window.onresize =
function ()
{
    console.scrollDown();
}

