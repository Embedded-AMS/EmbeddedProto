{#
Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved

This file is part of Embedded Proto.

Embedded Proto is open source software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3 of the license.

Embedded Proto  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.

For commercial and closed source application please visit:
<https://EmbeddedProto.com/license/>.

Embedded AMS B.V.
Info:
  info at EmbeddedProto dot com

Postal address:
  Atoomweg 2
  1627 LE, Hoorn
  the Netherlands
#}
{% import 'TypeOneof.h' as TypeOneof %}
{% for tmpl_param in typedef.get_templates() %}
{{"template<\n" if loop.first}}    {{tmpl_param['type']}} {{tmpl_param['name']}}{{", " if not loop.last}}{{"\n>" if loop.last}}
{% endfor %}
class {{ typedef.get_name() }} final: public ::EmbeddedProto::MessageInterface
{
  public:
    {{ typedef.get_name() }}() = default;
    {{ typedef.get_name() }}(const {{typedef.get_name()}}& rhs )
    {
      {% for field in typedef.fields %}
      {% if typedef.optional_fields is defined and field in typedef.optional_fields %}
      if(rhs.has_{{field.get_name()}}())
      {
        set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      }
      else
      {
        clear_{{ field.get_name() }}();
      }

      {% else %}
      set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      {% endif %}
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      {{ TypeOneof.assign(oneof)|indent(6) }}
      {% endfor %}
    }

    {{ typedef.get_name() }}(const {{typedef.get_name()}}&& rhs ) noexcept
    {
      {% for field in typedef.fields %}
      {% if typedef.optional_fields is defined and field in typedef.optional_fields %}
      if(rhs.has_{{field.get_name()}}())
      {
        set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      }
      else
      {
        clear_{{ field.get_name() }}();
      }

      {% else %}
      set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      {% endif %}
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      {{ TypeOneof.assign(oneof)|indent(6) }}
      {% endfor %}
    }

    ~{{ typedef.get_name() }}() override = default;

    {% for enum in typedef.nested_enum_definitions %}
    {{ enum.render(environment)|indent(4) }}

    {% endfor %}
    {% for msg in typedef.nested_msg_definitions %}
    {{ msg.render(environment)|indent(4) }}

    {% endfor %}
    enum class FieldNumber : uint32_t
    {
      NOT_SET = 0,
      {% for id_set in typedef.field_ids %}
      {{id_set[1]}} = {{id_set[0]}}{{ "," if not loop.last }}
      {% endfor %}
    };

    {{ typedef.name }}& operator=(const {{ typedef.name }}& rhs)
    {
      {% for field in typedef.fields %}
      {% if typedef.optional_fields is defined and field in typedef.optional_fields %}
      if(rhs.has_{{field.get_name()}}())
      {
        set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      }
      else
      {
        clear_{{ field.get_name() }}();
      }

      {% else %}
      set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      {% endif %}
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      {{ TypeOneof.assign(oneof)|indent(6) }}
      {% endfor %}
      return *this;
    }

    {{ typedef.name }}& operator=(const {{ typedef.name }}&& rhs) noexcept
    {
      {% for field in typedef.fields %}
      {% if typedef.optional_fields is defined and field in typedef.optional_fields %}
      if(rhs.has_{{field.get_name()}}())
      {
        set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      }
      else
      {
        clear_{{ field.get_name() }}();
      }
      
      {% else %}
      set_{{ field.get_name() }}(rhs.get_{{ field.get_name() }}());
      {% endif %}
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      {{ TypeOneof.assign(oneof)|indent(6) }}
      {% endfor %}
      return *this;
    }

    {% for field in typedef.fields %}
    {{ field.render_get_set(environment)|indent(4) }}

    {% endfor %}
    {% for oneof in typedef.oneofs %}
    FieldNumber get_which_{{oneof.get_name()}}() const { return {{oneof.get_which_oneof()}}; }

    {% for field in oneof.fields %}
    {{ field.render_get_set(environment)|indent(4) }}

    {% endfor %}
    {% endfor %}

    ::EmbeddedProto::Error serialize(::EmbeddedProto::WriteBufferInterface& buffer) const override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;

      {% for field in typedef.fields %}
      {{ field.render_serialize(environment)|indent(6) }}

      {% endfor %}
      {% for oneof in typedef.oneofs %}
      switch({{oneof.get_which_oneof()}})
      {
        {% for field in oneof.get_fields() %}
        case FieldNumber::{{field.variable_id_name}}:
          {{ field.render_serialize(environment)|indent(10) }}
          break;

        {% endfor %}
        default:
          break;
      }

      {% endfor %}
      return return_value;
    };

    ::EmbeddedProto::Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
      ::EmbeddedProto::WireFormatter::WireType wire_type = ::EmbeddedProto::WireFormatter::WireType::VARINT;
      uint32_t id_number = 0;
      FieldNumber id_tag = FieldNumber::NOT_SET;

      ::EmbeddedProto::Error tag_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id_number);
      while((::EmbeddedProto::Error::NO_ERRORS == return_value) && (::EmbeddedProto::Error::NO_ERRORS == tag_value))
      {
        id_tag = static_cast<FieldNumber>(id_number);
        switch(id_tag)
        {
          {% for field in typedef.fields %}
          case FieldNumber::{{field.get_variable_id_name()}}:
            {{ field.render_deserialize(environment)|indent(12) }}
            break;

          {% endfor %}
          {% for oneof in typedef.oneofs %}
          {% for field in oneof.get_fields() %}
          case FieldNumber::{{field.get_variable_id_name()}}:
          {% endfor %}
            return_value = deserialize_{{oneof.get_name()}}(id_tag, buffer, wire_type);
            break;

          {% endfor %}
          case FieldNumber::NOT_SET:
            return_value = ::EmbeddedProto::Error::INVALID_FIELD_ID;
            break;

          default:
            return_value = skip_unknown_field(buffer, wire_type);
            break;
        }

        if(::EmbeddedProto::Error::NO_ERRORS == return_value)
        {
          // Read the next tag.
          tag_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id_number);
        }
      }

      // When an error was detect while reading the tag but no other errors where found, set it in the return value.
      if((::EmbeddedProto::Error::NO_ERRORS == return_value)
         && (::EmbeddedProto::Error::NO_ERRORS != tag_value)
         && (::EmbeddedProto::Error::END_OF_BUFFER != tag_value)) // The end of the buffer is not an array in this case.
      {
        return_value = tag_value;
      }

      return return_value;
    };

    void clear() override
    {
      {% for field in typedef.fields %}
      clear_{{field.get_name()}}();
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      clear_{{oneof.get_name()}}();
      {% endfor %}

    }

    static char const* field_number_to_name(const FieldNumber fieldNumber)
    {
      char const* name = nullptr;
      switch(fieldNumber)
      {
        {% for field in typedef.fields %}
        case FieldNumber::{{field.get_variable_id_name()}}:
          name = {{field.get_name()|upper}}_NAME;
          break;
        {% endfor %}
        {% for oneof in typedef.oneofs %}
        {% for field in oneof.fields %}
        case FieldNumber::{{field.get_variable_id_name()}}:
          name = {{field.get_name()|upper}}_NAME;
          break;
        {% endfor %}
        {% endfor %}
        default:
          name = "Invalid FieldNumber";
          break;
      }
      return name;
    }

#ifdef MSG_TO_STRING

    ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str) const
    {
      return this->to_string(str, 0, nullptr, true);
    }

    ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str, const uint32_t indent_level, char const* name, const bool first_field) const override
    {
      ::EmbeddedProto::string_view left_chars = str;
      int32_t n_chars_used = 0;

      if(!first_field)
      {
        // Add a comma behind the previous field.
        n_chars_used = snprintf(left_chars.data, left_chars.size, ",\n");
        if(0 < n_chars_used)
        {
          // Update the character pointer and characters left in the array.
          left_chars.data += n_chars_used;
          left_chars.size -= n_chars_used;
        }
      }

      if(nullptr != name)
      {
        if( 0 == indent_level)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "\"%s\": {\n", name);
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\": {\n", indent_level, " ", name);
        }
      }
      else
      {
        if( 0 == indent_level)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "{\n");
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s{\n", indent_level, " ");
        }
      }
      
      if(0 < n_chars_used)
      {
        left_chars.data += n_chars_used;
        left_chars.size -= n_chars_used;
      }

      {% for field in typedef.fields %}
      {%if "FieldErrorRecursive" != field.descriptor.type_name %}{# Test if this is an FieldErrorRecursive #}
      left_chars = {{field.get_variable_name()}}.to_string(left_chars, indent_level + 2, {{field.get_name()|upper}}_NAME, {{ loop.first|lower }});
      {% endif %}
      {% endfor %}
      {% for oneof in typedef.oneofs %}
      left_chars = to_string_{{oneof.get_name()}}(left_chars, indent_level + 2, {{((typedef.fields|length == 0) and loop.first)|lower }});
      {% endfor %}  
      if( 0 == indent_level) 
      {
        n_chars_used = snprintf(left_chars.data, left_chars.size, "\n}");
      }
      else 
      {
        n_chars_used = snprintf(left_chars.data, left_chars.size, "\n%*s}", indent_level, " ");
      }

      if(0 < n_chars_used)
      {
        left_chars.data += n_chars_used;
        left_chars.size -= n_chars_used;
      }

      return left_chars;
    }

#endif // End of MSG_TO_STRING

  private:

      {% if typedef.optional_fields is defined and typedef.optional_fields|length > 0 %}
      // Define constants for tracking the presence of fields.
      // Use a struct to scope the variables from user fields as namespaces are not allowed within classes.
      struct presence
      {
        // An enumeration with all the fields for which presence has to be tracked.
        enum class fields : uint32_t
        {
          {% for field in typedef.optional_fields %}
          {{field.get_name().upper()}}{{ "," if not loop.last }}
          {% endfor %}
        };

        // The number of fields for which presence has to be tracked.
        static constexpr uint32_t N_FIELDS = {{typedef.optional_fields|length}};

        // Which type are we using to track presence.
        using TYPE = uint32_t;

        // How many bits are there in the presence type.
        static constexpr uint32_t N_BITS = std::numeric_limits<TYPE>::digits;

        // How many variables of TYPE do we need to bit mask all presence fields.
        static constexpr uint32_t SIZE = (N_FIELDS / N_BITS) + ((N_FIELDS % N_BITS) > 0 ? 1 : 0);

        // Obtain the index of a given field in the presence array.
        static constexpr uint32_t index(const fields& field) { return static_cast<uint32_t>(field) / N_BITS; }

        // Obtain the bit mask for the given field assuming we are at the correct index in the presence array.
        static constexpr TYPE mask(const fields& field)
        {
          return static_cast<uint32_t>(0x01) << (static_cast<uint32_t>(field) % N_BITS);
        }
      };

      // Create an array in which the presence flags are stored.
      typename presence::TYPE presence_[presence::SIZE] = {0};
      {% endif %}

      {% for field in typedef.fields %}
      {% if field.get_default_value() %}
      {{field.get_type()}} {{field.get_variable_name()}} = {{field.get_default_value()}};
      {% else %}
      {{field.get_type()}} {{field.get_variable_name()}};
      {% endif %}
      {% endfor %}

      {% for oneof in typedef.oneofs %}
      FieldNumber {{oneof.get_which_oneof()}} = FieldNumber::NOT_SET;
      union {{oneof.get_name()}}
      {
        {{oneof.get_name()}}() {}
        ~{{oneof.get_name()}}() {}
        {% for field in oneof.fields %}
        {# Here we use the field name variable instead of the get_ function as the get function will add the oneof
           name. This is the only place where this is required. #}
        {{field.get_type()}} {{field.variable_name}};
        {% endfor %}
      };
      {{oneof.get_name()}} {{oneof.get_variable_name()}};

      {{ TypeOneof.init(oneof)|indent(6) }}
      {{ TypeOneof.clear(oneof)|indent(6) }}
      {{ TypeOneof.deserialize(oneof, environment)|indent(6) }}
#ifdef MSG_TO_STRING 
      {{ TypeOneof.to_string(oneof)|indent(6) }}
#endif // End of MSG_TO_STRING
      {% endfor %}
};
