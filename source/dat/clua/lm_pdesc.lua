------------------------------------------------------------------------------
-- lm_desc.lua:
-- Portal descriptor markers.
------------------------------------------------------------------------------

require('clua/util.lua')

util.defclass("PortalDescriptor")

function PortalDescriptor:new(properties)
  local pd = util.newinstance(self)
  pd.props = properties
  return pd
end

function PortalDescriptor:write(marker, th)
  lmark.marshall_table(th, self.props)
end

function PortalDescriptor:read(marker, th)
  self.props = lmark.unmarshall_table(th)
  setmetatable(self, PortalDescriptor)
  return self
end

function PortalDescriptor:unmangle(x)
  if x and util.callable(x) then
    return x(self)
  else
    return x
  end
end

function PortalDescriptor:property(marker, pname)
  if pname == 'feature_description' then
    return self:unmangle(self.props.desc)
  elseif pname == 'feature_description_long' then
    return self:unmangle(self.props.desc_long)
  end

  return self:unmangle(self.props and self.props[pname] or '')
end

function portal_desc(props)
  return PortalDescriptor:new(props)
end
