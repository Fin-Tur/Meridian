<script setup>
import { computed } from 'vue'


const props = defineProps({
  title: {
    type: String,
    required: true
  },
  val: {
    type: Number,
    required: true
  },
  type: {
    type: String,
    default: ''
  },
  decimals: {
    type: Number,
    default: 0
  }
})

const text = computed(() => {
  let str = ''
  if(props.type.includes("beneficial") && props.vals >= 0){
    str += '+'
  }
  if(props.type.includes("percentile")){
    str += (props.val * 100).toFixed(props.decimals) + '%'
  }
  else if(props.type.includes("currency")){
    str += '$' + val.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })

  }else{
    str += props.val.toFixed(props.decimals)
  }
  return str
})

const text_color = computed(() => {
  if(props.type.includes("beneficial")){
    return props.val >= 0 ? 'text-positive' : 'text-negative'
  }else{
    return ''
  }
})

</script>

<template>
    <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">{{ props.title }}</div>
          <div :class="['text-2xl font-bold ', text_color]">{{text}}</div>
    </div>
</template>