#include "mvMenuItem.h"
#include "mvContext.h"
#include "mvItemRegistry.h"
#include "mvPythonExceptions.h"
#include "AppItems/fonts/mvFont.h"
#include "AppItems/themes/mvTheme.h"
#include "AppItems/containers/mvDragPayload.h"

namespace Marvel {

	mvMenuItem::mvMenuItem(mvUUID uuid)
		: mvAppItem(uuid) 
	{
	}

	void mvMenuItem::applySpecificTemplate(mvAppItem* item)
	{
		auto titem = static_cast<mvMenuItem*>(item);
		if (_source != 0) _value = titem->_value;
		_disabled_value = titem->_disabled_value;
		_shortcut = titem->_shortcut;
		_check = titem->_check;
	}

	void mvMenuItem::draw(ImDrawList* drawlist, float x, float y)
	{


		//-----------------------------------------------------------------------------
		// pre draw
		//-----------------------------------------------------------------------------

		// show/hide
		if (!_show)
			return;

		// focusing
		if (_focusNextFrame)
		{
			ImGui::SetKeyboardFocusHere();
			_focusNextFrame = false;
		}

		// cache old cursor position
		ImVec2 previousCursorPos = ImGui::GetCursorPos();

		// set cursor position if user set
		if (_dirtyPos)
			ImGui::SetCursorPos(_state.pos);

		// update widget's position state
		_state.pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };

		// set item width
		if (_width != 0)
			ImGui::SetNextItemWidth((float)_width);

		// set indent
		if (_indent > 0.0f)
			ImGui::Indent(_indent);

		// push font if a font object is attached
		if (_font)
		{
			ImFont* fontptr = static_cast<mvFont*>(_font.get())->getFontPtr();
			ImGui::PushFont(fontptr);
		}

		// themes
		apply_local_theming(this);

		//-----------------------------------------------------------------------------
		// draw
		//-----------------------------------------------------------------------------
		{

			ScopedID id(_uuid);

			// This is ugly and goes against our style system but its the only widget that ImGui chooses to push the disable color for us
			// so we have to map our text disable color to the system text disable color, or we can create a new constant which goes agains our 
			// constants. 
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::GetStyleColorVec4(ImGuiCol_Text));

			// create menu item and see if its selected
			if (ImGui::MenuItem(_internalLabel.c_str(), _shortcut.c_str(), _check ? _value.get() : nullptr, _enabled))
			{
				bool value = *_value;

				if(_alias.empty())
					mvAddCallback(_callback, _uuid, ToPyBool(value), _user_data);
				else
					mvAddCallback(_callback, _alias, ToPyBool(value), _user_data);
			}

			ImGui::PopStyleColor();

		}

		//-----------------------------------------------------------------------------
		// update state
		//-----------------------------------------------------------------------------
		UpdateAppItemState(_state);

		//-----------------------------------------------------------------------------
		// post draw
		//-----------------------------------------------------------------------------

		// set cursor position to cached position
		if (_dirtyPos)
			ImGui::SetCursorPos(previousCursorPos);

		if (_indent > 0.0f)
			ImGui::Unindent(_indent);

		// pop font off stack
		if (_font)
			ImGui::PopFont();

		// handle popping themes
		cleanup_local_theming(this);

		if (_handlerRegistry)
			_handlerRegistry->customAction(&_state);

		// handle drag & drop if used
		apply_drag_drop(this);
	}

	void mvMenuItem::handleSpecificKeywordArgs(PyObject* dict)
	{
		if (dict == nullptr)
			return;

		if (PyObject* item = PyDict_GetItemString(dict, "shortcut")) _shortcut = ToString(item);
		if (PyObject* item = PyDict_GetItemString(dict, "check")) _check = ToBool(item);

}

	void mvMenuItem::getSpecificConfiguration(PyObject* dict)
	{
		if (dict == nullptr)
			return;

		PyDict_SetItemString(dict, "shortcut", mvPyObject(ToPyString(_shortcut)));
		PyDict_SetItemString(dict, "check", mvPyObject(ToPyBool(_check)));
	}

	PyObject* mvMenuItem::getPyValue()
	{
		return ToPyBool(*_value);
	}

	void mvMenuItem::setPyValue(PyObject* value)
	{
		*_value = ToBool(value);
	}

	void mvMenuItem::setDataSource(mvUUID dataSource)
	{
		if (dataSource == _source) return;
		_source = dataSource;

		mvAppItem* item = GetItem((*GContext->itemRegistry), dataSource);
		if (!item)
		{
			mvThrowPythonError(mvErrorCode::mvSourceNotFound, "set_value",
				"Source item not found: " + std::to_string(dataSource), this);
			return;
		}
		if (GetEntityValueType(item->_type) != GetEntityValueType(_type))
		{
			mvThrowPythonError(mvErrorCode::mvSourceNotCompatible, "set_value",
				"Values types do not match: " + std::to_string(dataSource), this);
			return;
		}
		_value = *static_cast<std::shared_ptr<bool>*>(item->getValue());
	}
}