#pragma once
#define AS_BOOL( value ) ((value).m_Data.m_Bool)
#define AS_NUMBER( value ) ((value).m_Data.m_Number)

#define MAKE_NULL ((QScript::Value){ QScript::VT_NULL, { .m_Number = 0 } })
#define MAKE_BOOL( value ) ((QScript::Value){ QScript::VT_BOOL, { .m_Bool = value } })
#define MAKE_NUMBER( value ) ((QScript::Value){ QScript::VT_NUMBER, { .m_Number = value } })

#define IS_NULL( value ) ((value).m_Type == QScript::VT_NULL)
#define IS_BOOL( value ) ((value).m_Type == QScript::VT_BOOL)
#define IS_NUMBER( value ) ((value).m_Type == QScript::VT_NUMBER)

namespace QScript
{
	enum ValueType
	{
		VT_BOOL = 0,
		VT_NUMBER,
		VT_NULL,
	};

	struct Value
	{
		ValueType 	m_Type;
		union {
			bool	m_Bool;
			double	m_Number;
		} m_Data;
	};
}
