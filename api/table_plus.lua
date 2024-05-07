local table_plus = {}
table_plus.__index = table

function table_plus:length(table)
    local len = 0
    for i, v in ipairs(table) do
        len = len + 1
    end
    return len
end
return table_plus