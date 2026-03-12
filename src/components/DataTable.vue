<script setup>
/**
 * DataTable – minimalist, prop-driven table component.
 *
 * Props
 * ─────
 * title    (String)  – optional card header
 * columns  (Array)   – [{ key, label?, align?, headerClass?, cellClass? }]
 * rows     (Array)   – array of objects (keyed by column.key) OR arrays
 * hover    (Boolean) – card hover effect (default true)
 *
 * Slots
 * ─────
 * #cell-<key>  ({ value, row, rowIndex, col, colIndex })  – per-column override
 * #cell        ({ value, row, rowIndex, col, colIndex })  – fallback override
 * #row-after   ({ row, rowIndex })                        – extra content after <td>s
 */
defineProps({
  title:   { type: String,  default: '' },
  columns: { type: Array,   required: true },
  rows:    { type: Array,   required: true },
  hover:   { type: Boolean, default: true },
})

function cellValue(row, col) {
  if (Array.isArray(row)) return row[col._index]
  return row[col.key]
}
</script>

<template>
  <div
    class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition"
    :class="{ 'hover:bg-bg-card-hover': hover }"
  >
    <div v-if="title" class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">
      {{ title }}
    </div>

    <div class="overflow-x-auto">
      <table class="w-full text-sm">
        <thead>
          <tr>
            <th
              v-for="(col, ci) in columns"
              :key="ci"
              class="px-4 py-3 font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border"
              :class="[
                col.align === 'center' ? 'text-center' : col.align === 'right' ? 'text-right' : 'text-left',
                col.headerClass ?? ''
              ]"
            >
              {{ col.label ?? '' }}
            </th>
          </tr>
        </thead>

        <tbody>
          <tr v-for="(row, ri) in rows" :key="ri">
            <td
              v-for="(col, ci) in columns"
              :key="ci"
              class="px-4 py-3 border-b border-border"
              :class="[
                col.align === 'center' ? 'text-center' : col.align === 'right' ? 'text-right' : 'text-left',
                col.cellClass ?? ''
              ]"
              :style="col.cellStyle ? col.cellStyle(cellValue(row, { ...col, _index: ci }), row, ri) : undefined"
            >
              <!-- per-column slot -->
              <slot
                :name="'cell-' + col.key"
                :value="cellValue(row, { ...col, _index: ci })"
                :row="row"
                :rowIndex="ri"
                :col="col"
                :colIndex="ci"
              >
                <!-- general fallback slot -->
                <slot
                  name="cell"
                  :value="cellValue(row, { ...col, _index: ci })"
                  :row="row"
                  :rowIndex="ri"
                  :col="col"
                  :colIndex="ci"
                >
                  {{ cellValue(row, { ...col, _index: ci }) }}
                </slot>
              </slot>
            </td>

            <!-- optional extra cells per row (e.g. action buttons) -->
            <slot name="row-after" :row="row" :rowIndex="ri" />
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>
